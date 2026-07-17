/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Offscreen pin for the Plasma 6 shortcuts-host discovery - the test the
//! porting plan's keyboard item owed since the 2026-07-16 inventory
//! correction. Both halves it pins broke SILENTLY once: the walk kept the
//! Plasma 5 child-scan shape while itemForApplet started handing back the
//! containment root that IS containmentViewLayout, so the host never
//! resolved and every shortcuts-host method (activateEntryAtIndex,
//! newInstanceForEntryAtIndex, setShowAppletShortcutBadges,
//! appletIdForIndex) no-op'd - Meta+number only looked alive through
//! fallbacks.
//!
//! The containment graph is real: a concrete Corona parents a Panel
//! containment (the layoutmanagerparkingtest corona recipe) whose main
//! script rides the qrc applet path, and the graphic item is built through
//! the REAL creation path, PlasmaQuick::AppletQuickItem::itemForApplet().
//! The ability host inside the harness is the REAL shipped
//! abilities/PositionShortcuts.qml (+ its private), aliased into the qrc
//! from containment/package - so this fails when the Plasma tree shape
//! changes again, when the objectNames move, or when the QML host's
//! function signatures drift away from the C++ QMetaMethod strings.
//!
//! The walk and the signature resolution run through the shipped statics
//! (Latte::ViewPart::ContainmentInterface::findShortcutsHost /
//! resolveShortcutsHostMethods), the same code identifyShortcutsHost
//! executes in the dock.

// Qt
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

// KDE
#include <KPluginMetaData>

// Plasma
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <PlasmaQuick/AppletQuickItem>

// the class under test, from lattedock-core
#include <view/containmentinterface.h>

namespace {

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

class ShortcutsHostTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void discoveryFindsRealHostAndAllFourSignaturesResolve();
    void childScanFallbackStillWalks();
    void hostlessGraphResolvesToNothing();

private:
    QTemporaryDir m_latteModulesRoot;
};

void ShortcutsHostTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    //! same environment rules as the contract suite: imports resolve ONLY
    //! from the devShell's pinned module set, and the session bus stays
    //! unreachable
    const QByteArray pinnedModules = qgetenv("LATTE_QML_MODULE_PATH");
    QVERIFY2(!pinnedModules.isEmpty(),
             "LATTE_QML_MODULE_PATH is not set - this test must run inside the flake devShell (nix develop), like the QML harnesses");
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null");

    //! qrc URLs carry no timestamp, so the engine's on-disk .qmlc cache
    //! can serve a STALE compilation of the aliased real files and mask
    //! exactly the drift this pin exists to catch - proven at
    //! introduction: with the cache on, a deliberately drifted
    //! appletIdForIndex signature still passed; with it off, it failed
    qputenv("QML_DISABLE_DISK_CACHE", "1");

    //! the real PositionShortcutsPrivate.qml imports
    //! org.kde.latte.abilities.definition - a pure-QML module that lives
    //! in the source tree; expose exactly that one leaf through a private
    //! import root (never a shared tree, per the regression-discipline
    //! allow-list rule)
    QVERIFY(m_latteModulesRoot.isValid());
    QVERIFY(QDir().mkpath(m_latteModulesRoot.path() + QStringLiteral("/org/kde/latte/abilities")));
    QVERIFY(QFile::link(QStringLiteral(REPO_ROOT "/declarativeimports/abilities/definition"),
                        m_latteModulesRoot.path() + QStringLiteral("/org/kde/latte/abilities/definition")));

    qputenv("QML2_IMPORT_PATH", QByteArray(pinnedModules + ':' + m_latteModulesRoot.path().toUtf8()));
}

void ShortcutsHostTest::discoveryFindsRealHostAndAllFourSignaturesResolve()
{
    TestCorona corona;

    QJsonObject hostJson{
        {QStringLiteral("KPlugin"), QJsonObject{{QStringLiteral("Id"), QStringLiteral("test.host")}, {QStringLiteral("Name"), QStringLiteral("Host")}}},
        {QStringLiteral("X-Plasma-ContainmentType"), QStringLiteral("Panel")},
    };
    auto *containment = new Plasma::Containment(&corona, KPluginMetaData(hostJson, QString()), {QVariant(), QVariant(1)});
    QVERIFY(containment->isContainment());

    //! the REAL creation path identifyShortcutsHost walks from
    QQuickItem *graphicItem = PlasmaQuick::AppletQuickItem::itemForApplet(containment);
    QVERIFY2(graphicItem, "itemForApplet must build the ContainmentItem from the qrc main script");

    //! the Plasma 6 tree shape the 2026-07-16 fix keys on: the root
    //! itself carries the objectName (Plasma 5 wrapped it one level down)
    QCOMPARE(graphicItem->objectName(), QStringLiteral("containmentViewLayout"));

    QQuickItem *host = Latte::ViewPart::ContainmentInterface::findShortcutsHost(graphicItem);
    QVERIFY2(host, "the discovery walk must find the real PositionShortcuts ability host");
    QCOMPARE(host->objectName(), QStringLiteral("PositionShortcutsAbilityHost"));

    //! all four invokable signatures, resolved against the REAL shipped
    //! QML - the exact QMetaMethod strings the C++ invokes with
    const auto methods = Latte::ViewPart::ContainmentInterface::resolveShortcutsHostMethods(host);
    QVERIFY2(methods.activateEntryAtIndex.isValid(), "activateEntryAtIndex(QVariant) must resolve on the shipped host");
    QVERIFY2(methods.newInstanceForEntryAtIndex.isValid(), "newInstanceForEntryAtIndex(QVariant) must resolve on the shipped host");
    QVERIFY2(methods.setShowAppletShortcutBadges.isValid(), "setShowAppletShortcutBadges(QVariant,QVariant,QVariant,QVariant) must resolve on the shipped host");
    QVERIFY2(methods.appletIdForIndex.isValid(), "appletIdForIndex(QVariant) must resolve on the shipped host");

    delete containment;
}

void ShortcutsHostTest::childScanFallbackStillWalks()
{
    //! the Plasma 5 shape kept as a safety net (mirroring Panel.qml): the
    //! layout is a CHILD of the handed-back root; plain items suffice -
    //! this pins the walk, the real-graph test above pins the signatures
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        Item {
            Item {
                objectName: "containmentViewLayout"
                Item { objectName: "PositionShortcutsAbilityHost" }
            }
        }
    )", QUrl(QStringLiteral("shortcutshostfallback.qml")));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    QQuickItem *host = Latte::ViewPart::ContainmentInterface::findShortcutsHost(qobject_cast<QQuickItem *>(root.data()));
    QVERIFY(host);
    QCOMPARE(host->objectName(), QStringLiteral("PositionShortcutsAbilityHost"));
}

void ShortcutsHostTest::hostlessGraphResolvesToNothing()
{
    QQuickItem plainRoot;
    QCOMPARE(Latte::ViewPart::ContainmentInterface::findShortcutsHost(&plainRoot), nullptr);
    QCOMPARE(Latte::ViewPart::ContainmentInterface::findShortcutsHost(nullptr), nullptr);

    //! a hostless graph also resolves no methods - invalid, never a crash
    const auto methods = Latte::ViewPart::ContainmentInterface::resolveShortcutsHostMethods(nullptr);
    QVERIFY(!methods.activateEntryAtIndex.isValid());
    QVERIFY(!methods.newInstanceForEntryAtIndex.isValid());
    QVERIFY(!methods.setShowAppletShortcutBadges.isValid());
    QVERIFY(!methods.appletIdForIndex.isValid());
}

QTEST_MAIN(ShortcutsHostTest)

#include "shortcutshosttest.moc"
