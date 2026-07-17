/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Upstream contract (tests/contracts/README.md): where the `expanded`
//! property lives at the pinned libplasma.
//!
//! Qt5's Containment::applets() handed back the applets' GRAPHIC objects,
//! so the containment QML could write `Plasmoid.applets[i].expanded = false`
//! to collapse every popup when entering configure mode. On Plasma 6 the
//! same call returns Plasma::Applet objects (libplasma's containment.h
//! carries a "KF6: this should be AppletQuickItem *" TODO on the property)
//! and `expanded` lives one hop away on the graphic item, so the Qt5-shaped
//! loop throws a TypeError on the first applet and collapses nothing - the
//! moved-one-hop family (c3d15966), aggravated by handler-aborting
//! TypeErrors (the family-3 shape).
//!
//! The fix routes through ContainmentInterface::deactivateApplets(), which
//! does the applet-to-item hop in C++ via itemForApplet()->setExpanded().
//! This test pins both premises:
//!   1. Plasma::Applet has NO `expanded` property (if a libplasma bump ever
//!      adds one back, the QML-side loop becomes possible again and this
//!      routing decision should be re-audited);
//!   2. the graphic item built by itemForApplet DOES carry a writable
//!      `expanded` (what deactivateApplets relies on). Only the property's
//!      presence and writability are pinned - the collapse round-trip is
//!      scene-dependent (see the note at the assertion).

// Qt
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QStandardPaths>
#include <QtTest>

// KDE
#include <KPluginMetaData>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <PlasmaQuick/AppletQuickItem>

namespace {

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

} // namespace

class AppletsExpandedPropertyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void appletsListCarriesAppletsNotGraphicItems();
    void expandedLivesOnTheGraphicItemOnly();

private:
    Plasma::Containment *createContainment(Plasma::Corona *corona);
    Plasma::Applet *createApplet(Plasma::Containment *host, int id);
};

void AppletsExpandedPropertyTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null");
}

Plasma::Containment *AppletsExpandedPropertyTest::createContainment(Plasma::Corona *corona)
{
    QJsonObject json{
        {QStringLiteral("KPlugin"), QJsonObject{{QStringLiteral("Id"), QStringLiteral("test.host")}, {QStringLiteral("Name"), QStringLiteral("Host")}}},
        {QStringLiteral("X-Plasma-ContainmentType"), QStringLiteral("Panel")},
    };
    auto *containment = new Plasma::Containment(corona, KPluginMetaData(json, QString()), {QVariant(), QVariant(1)});
    return containment;
}

Plasma::Applet *AppletsExpandedPropertyTest::createApplet(Plasma::Containment *host, int id)
{
    QJsonObject json{
        {QStringLiteral("KPlugin"), QJsonObject{{QStringLiteral("Id"), QStringLiteral("test.expandapplet")}, {QStringLiteral("Name"), QStringLiteral("Expand Applet")}}},
    };
    auto *applet = new Plasma::Applet(host, KPluginMetaData(json, QString()), {QVariant(), QVariant(id)});
    host->addApplet(applet);
    applet->updateConstraints(Plasma::Applet::StartupCompletedConstraint);
    applet->config();
    return applet;
}

void AppletsExpandedPropertyTest::appletsListCarriesAppletsNotGraphicItems()
{
    QObject arena;
    auto *corona = new TestCorona(&arena);
    Plasma::Containment *host = createContainment(corona);
    QVERIFY(host->isContainment());

    Plasma::Applet *applet = createApplet(host, 7);
    QVERIFY(applet);
    QCOMPARE(host->applets().size(), 1);

    //! the list element is the Plasma::Applet itself - the object the QML
    //! `Plasmoid.applets` enumeration hands to bindings
    QCOMPARE(host->applets().constFirst(), applet);

    //! ...and that object has NO `expanded`: a QML write to it throws a
    //! TypeError that aborts the surrounding handler
    QCOMPARE(applet->metaObject()->indexOfProperty("expanded"), -1);
}

void AppletsExpandedPropertyTest::expandedLivesOnTheGraphicItemOnly()
{
    QObject arena;
    auto *corona = new TestCorona(&arena);
    Plasma::Containment *host = createContainment(corona);
    Plasma::Applet *applet = createApplet(host, 8);
    QVERIFY(applet);

    PlasmaQuick::AppletQuickItem *item = PlasmaQuick::AppletQuickItem::itemForApplet(applet);
    QVERIFY2(item, "itemForApplet returned no graphic item at the pin");

    const int expandedIndex = item->metaObject()->indexOfProperty("expanded");
    QVERIFY2(expandedIndex != -1, "the graphic item lost its expanded property at the pin");
    QVERIFY(item->metaObject()->property(expandedIndex).isWritable());

    //! the write resolves through the property system (setExpanded is a plain
    //! guarded setter at the pin). The false direction is NOT pinned here:
    //! compactRepresentationCheck() forces expanded back to true whenever the
    //! full representation is shown in place (appletquickitem.cpp), which is
    //! scene-dependent behavior an offscreen fixture cannot honestly drive -
    //! the collapse itself is verified live through the autohide path that
    //! shares deactivateApplets().
    QVERIFY(item->setProperty("expanded", true));
    QCOMPARE(item->property("expanded").toBool(), true);
}

QTEST_MAIN(AppletsExpandedPropertyTest)

#include "appletsexpandedpropertytest.moc"
