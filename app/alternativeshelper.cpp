/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 Bree Spektor

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "alternativeshelper.h"

// Qt
#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>
#include <QRectF>

// KDE
#include <KPackage/Package>
#include <kconfig_version.h>

// Plasma
#include <Plasma/Containment>
#include <Plasma/PluginLoader>
#include <PlasmaQuick/AppletQuickItem>

AlternativesHelper::AlternativesHelper(Plasma::Applet *applet, QObject *parent)
    : QObject(parent),
      m_applet(applet)
{
}

AlternativesHelper::~AlternativesHelper()
{
}

QStringList AlternativesHelper::appletProvides() const
{
    return m_applet->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"), QStringList());
}

QString AlternativesHelper::currentPlugin() const
{
    return m_applet->pluginMetaData().pluginId();
}

QQuickItem *AlternativesHelper::applet() const
{
    //! Plasma 6 removed the _plasma_graphicObject property; the graphic item
    //! is resolved through AppletQuickItem (same migration as View::init,
    //! spotted independently by latte-dock-ng 613ddcc3b)
    return PlasmaQuick::AppletQuickItem::itemForApplet(m_applet);
}

void AlternativesHelper::loadAlternative(const QString &plugin)
{
    if (plugin == currentPlugin() || m_applet->isContainment()) {
        return;
    }

    Plasma::Containment *cont = m_applet->containment();

    if (!cont) {
        return;
    }

    QQuickItem *appletItem = PlasmaQuick::AppletQuickItem::itemForApplet(m_applet);
    QQuickItem *contItem = PlasmaQuick::AppletQuickItem::itemForApplet(cont);

    if (!appletItem || !contItem) {
        //! reading _plasma_graphicObject here used to null out SILENTLY on
        //! Plasma 6 and eat the user's alternative selection; if the resolved
        //! items are ever missing again, say so
        qWarning() << "AlternativesHelper::loadAlternative could not resolve the applet/containment quick items;"
                   << "alternative" << plugin << "was NOT applied";
        return;
    }

    // ensure the global shortcut is moved to the new applet
    const QKeySequence &shortcut = m_applet->globalShortcut();
    m_applet->setGlobalShortcut(QKeySequence()); // need to unmap the old one first

    const QPointF newPos = appletItem->mapToItem(contItem, QPointF(0, 0));

    m_applet->destroy();

    connect(m_applet, &QObject::destroyed, [ = ]() {
        Plasma::Applet *newApplet = Q_NULLPTR;
        //! Plasma 6's ContainmentItem::createApplet takes a QRectF geometry
        //! hint where Qt5's took a QPoint. invokeMethod resolves by exact
        //! argument types, so a QPoint Q_ARG fails to resolve and the call
        //! silently no-ops - AFTER the old applet was already destroyed, so
        //! the applet just vanished instead of being swapped. A zero size
        //! keeps the Qt5 position-only semantics (ContainmentItem only adopts
        //! geom.size() when both dimensions are positive). Pinned by
        //! tests/contracts/alternativescreateapplettest.cpp.
        const bool invoked = QMetaObject::invokeMethod(contItem, "createApplet",
                                                       Q_RETURN_ARG(Plasma::Applet *, newApplet),
                                                       Q_ARG(QString, plugin),
                                                       Q_ARG(QVariantList, QVariantList()),
                                                       Q_ARG(QRectF, QRectF(newPos, QSizeF(0, 0))));

        if (!invoked || !newApplet) {
            qWarning() << "AlternativesHelper: createApplet did not produce the alternative" << plugin
                       << "- the previous applet is already destroyed, nothing replaced it";
            return;
        }

        newApplet->setGlobalShortcut(shortcut);
    });
}

#include "moc_alternativeshelper.cpp"

