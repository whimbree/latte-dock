/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Upstream contract (tests/contracts/README.md): the edit-mode
//! entry/exit detection premise at the pinned libplasma v6.6.5.
//!
//! Latte's whole edit-mode detection is one property with one writer:
//! PrimaryConfigView calls Containment::setUserConfiguring(true/false)
//! (primaryconfigview.cpp), the containment QML binds
//! `editMode: Plasmoid.userConfiguring` (containment main.qml), and
//! View re-broadcasts Containment::userConfiguringChanged as
//! inEditModeChanged (view.cpp). This works because at the pin
//! Applet::setUserConfiguring is a PLAIN GUARDED SETTER: synchronous,
//! no event-loop deferral, no signal compression beyond the equal-value
//! guard (applet.cpp). latte-dock-ng burned 8+ attempts (polling
//! timers, overlays, C++ interception) on the theory that this
//! notification is unreliable on Plasma 6; reading the pinned source
//! and this port's live history says the notification is fine and
//! their bugs lived elsewhere (stale persisted sub-modes, chrome focus
//! races - fixed here as fb621102 and 4a8ac480).
//!
//! If a libplasma bump makes any of these assertions fail (deferred
//! emission, lost re-entrant writes, emission on equal value), the
//! edit-mode detection design must be re-audited before the bump.

// Qt
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest>

// KDE
#include <KPluginMetaData>

// Plasma
#include <Plasma/Containment>
#include <Plasma/Corona>

//! Corona's one pure virtual is screenGeometry(); everything else works
//! offscreen as-is
class TestCorona : public Plasma::Corona
{
    Q_OBJECT

public:
    using Plasma::Corona::Corona;

    QRect screenGeometry(int id) const override
    {
        Q_UNUSED(id);
        return QRect(0, 0, 1920, 1080);
    }
};

class UserConfiguringContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void notifiesSynchronouslyOnEveryRealChange();
    void equalValueWriteEmitsNothing();

private:
    Plasma::Containment *createContainment(Plasma::Corona *corona);
};

void UserConfiguringContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null");
}

Plasma::Containment *UserConfiguringContractTest::createContainment(Plasma::Corona *corona)
{
    QJsonObject json{
        {QStringLiteral("KPlugin"),
         QJsonObject{{QStringLiteral("Id"), QStringLiteral("org.kde.test.panel")},
                     {QStringLiteral("Name"), QStringLiteral("Test Panel")}}},
        {QStringLiteral("X-Plasma-ContainmentType"), QStringLiteral("Panel")},
    };

    auto *containment = new Plasma::Containment(corona, KPluginMetaData(json, QString()), {QVariant(), QVariant(1)});
    containment->init();
    return containment;
}

void UserConfiguringContractTest::notifiesSynchronouslyOnEveryRealChange()
{
    TestCorona corona;
    auto *containment = createContainment(&corona);

    QSignalSpy spy(containment, &Plasma::Applet::userConfiguringChanged);

    containment->setUserConfiguring(true);

    //! synchronous: the emission happened inside the setter call, before
    //! any event loop ran (QSignalSpy would be empty here if the pin ever
    //! deferred the signal)
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    QVERIFY(containment->isUserConfiguring());

    containment->setUserConfiguring(false);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), false);
    QVERIFY(!containment->isUserConfiguring());
}

void UserConfiguringContractTest::equalValueWriteEmitsNothing()
{
    TestCorona corona;
    auto *containment = createContainment(&corona);

    QSignalSpy spy(containment, &Plasma::Applet::userConfiguringChanged);

    //! the equal-value guard: re-asserting the current state is a no-op,
    //! so a redundant writer cannot flap edit mode
    containment->setUserConfiguring(false);
    QCOMPARE(spy.count(), 0);

    containment->setUserConfiguring(true);
    containment->setUserConfiguring(true);
    QCOMPARE(spy.count(), 1);
}

//! QTEST_MAIN, not GUILESS: Plasma::CoronaPrivate::init() dereferences the
//! gui application (crashes under a bare QCoreApplication)
QTEST_MAIN(UserConfiguringContractTest)
#include "userconfiguringcontracttest.moc"
