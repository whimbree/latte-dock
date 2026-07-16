/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dropclassifiertools.h"

// local
#include "units/dropclassifier.h"

// Qt
#include <QDebug>
#include <QMimeData>
#include <QUrl>

namespace Latte {

DropClassifierTools::DropClassifierTools(QObject *parent)
    : QObject(parent)
{
}

namespace {

//! the wrapper marshals the event's QMimeData once per handler call;
//! the payload text conversion matches what the QML engine did for the
//! Qt5-era String(getDataAsByteArray(...)) read (verified against the
//! pinned Qt: QByteArray -> JS string is fromUtf8)
DropClassifier::MimeSnapshot toMimeSnapshot(const QMimeData *mimeData)
{
    return DropClassifier::MimeSnapshot{
        .formats = mimeData->formats(),
        .plasmoidServiceData = QString::fromUtf8(mimeData->data(QString(DropClassifier::PlasmoidServiceNameFormat))),
        .hasUrls = mimeData->hasUrls(),
        .urls = mimeData->urls(),
    };
}

//! the isApplication predicate arrives as a JS callable (it consults
//! live C++ - extendedInterface/backend.isApplication). Invoked
//! synchronously, never stored. The shipped JS passed url STRINGS
//! (DeclarativeMimeData.urls stringifies), so the callable receives
//! url.toString(). A JS exception is surfaced, never swallowed: the
//! url answers "not an application", which fails every() exactly as a
//! false answer would.
auto jsApplicationUrlPredicate(const QJSValue &isApplicationUrl)
{
    return [&isApplicationUrl](const QUrl &url) -> bool {
        QJSValue callable = isApplicationUrl; // call() is non-const; QJSValue copies are handles
        const QJSValue answer = callable.call({QJSValue(url.toString())});

        if (answer.isError()) {
            qWarning() << "DropClassifier: isApplication predicate threw for" << url
                       << ":" << answer.toString() << "- treating as not an application";
            return false;
        }

        return answer.toBool();
    };
}

bool refuseBadClassifyInputs(const char *where, const QMimeData *mimeData, const QJSValue &isApplicationUrl)
{
    if (!mimeData) {
        qCritical() << "DropClassifier:" << where << "called with null mimeData - refusing to classify";
        return true;
    }

    if (!isApplicationUrl.isCallable()) {
        qCritical() << "DropClassifier:" << where
                    << "called without a callable isApplication predicate - refusing to classify";
        return true;
    }

    return false;
}

}

QVariantMap DropClassifierTools::classifyContainmentDrag(QMimeData *mimeData, const QJSValue &isApplicationUrl)
{
    if (refuseBadClassifyInputs("classifyContainmentDrag", mimeData, isApplicationUrl)) {
        //! the inert answer: an all-false classification routes to the
        //! plain InsertSpacer/ProcessMime paths exactly like an
        //! unrecognized drag
        return {{QStringLiteral("isTask"), false},
                {QStringLiteral("isPlasmoid"), false},
                {QStringLiteral("isSeparator"), false},
                {QStringLiteral("isLatteTasks"), false},
                {QStringLiteral("onlyLaunchers"), false}};
    }

    const DropClassifier::ContainmentDragFlags flags =
            DropClassifier::classifyContainmentDrag(toMimeSnapshot(mimeData),
                                                    jsApplicationUrlPredicate(isApplicationUrl));

    return {{QStringLiteral("isTask"), flags.isTask},
            {QStringLiteral("isPlasmoid"), flags.isPlasmoid},
            {QStringLiteral("isSeparator"), flags.isSeparator},
            {QStringLiteral("isLatteTasks"), flags.isLatteTasks},
            {QStringLiteral("onlyLaunchers"), flags.onlyLaunchers}};
}

QVariantMap DropClassifierTools::classifyTasksDrag(QMimeData *mimeData, const QJSValue &isApplicationUrl)
{
    if (refuseBadClassifyInputs("classifyTasksDrag", mimeData, isApplicationUrl)) {
        //! all-false fails tasksDragAccepts, so the shell ignores the
        //! event - the same refusal an unrecognized drag gets
        return {{QStringLiteral("movingTask"), false},
                {QStringLiteral("droppingOnlyLaunchers"), false},
                {QStringLiteral("droppingSeparator"), false},
                {QStringLiteral("droppingFiles"), false}};
    }

    const DropClassifier::TasksDropFlags flags =
            DropClassifier::classifyTasksDrag(toMimeSnapshot(mimeData),
                                              jsApplicationUrlPredicate(isApplicationUrl));

    return {{QStringLiteral("movingTask"), flags.movingTask},
            {QStringLiteral("droppingOnlyLaunchers"), flags.droppingOnlyLaunchers},
            {QStringLiteral("droppingSeparator"), flags.droppingSeparator},
            {QStringLiteral("droppingFiles"), flags.droppingFiles}};
}

int DropClassifierTools::containmentDragEnterAction(bool isTask, bool onlyLaunchers, bool immutable,
                                                    bool viewShownFully, bool hasStealingApplet)
{
    const DropClassifier::ContainmentDragFlags flags{.isTask = isTask, .onlyLaunchers = onlyLaunchers};

    switch (DropClassifier::containmentDragEnterAction(flags, immutable, viewShownFully, hasStealingApplet)) {
    case DropClassifier::ContainmentEnterAction::Reject:
        return EnterReject;
    case DropClassifier::ContainmentEnterAction::ShowAddLaunchersMessage:
        return EnterShowAddLaunchersMessage;
    case DropClassifier::ContainmentEnterAction::InsertSpacer:
        return EnterInsertSpacer;
    }

    Q_UNREACHABLE();
}

int DropClassifierTools::containmentDragMoveAction(bool isTask, bool onlyLaunchers, bool hasStealingApplet)
{
    const DropClassifier::ContainmentDragFlags flags{.isTask = isTask, .onlyLaunchers = onlyLaunchers};

    switch (DropClassifier::containmentDragMoveAction(flags, hasStealingApplet)) {
    case DropClassifier::ContainmentMoveAction::LeaveUnchanged:
        return MoveLeaveUnchanged;
    case DropClassifier::ContainmentMoveAction::ShowAddLaunchersMessage:
        return MoveShowAddLaunchersMessage;
    case DropClassifier::ContainmentMoveAction::InsertSpacer:
        return MoveInsertSpacer;
    }

    Q_UNREACHABLE();
}

int DropClassifierTools::containmentDropAction(bool isTask, bool onlyLaunchers, bool viewShownFully,
                                               bool hasStealingApplet)
{
    const DropClassifier::ContainmentDragFlags flags{.isTask = isTask, .onlyLaunchers = onlyLaunchers};

    switch (DropClassifier::containmentDropAction(flags, viewShownFully, hasStealingApplet)) {
    case DropClassifier::ContainmentDropAction::Ignore:
        return DropIgnore;
    case DropClassifier::ContainmentDropAction::AddLaunchersToStealingApplet:
        return DropAddLaunchersToStealingApplet;
    case DropClassifier::ContainmentDropAction::ProcessMime:
        return DropProcessMime;
    }

    Q_UNREACHABLE();
}

bool DropClassifierTools::tasksDragAccepts(bool movingTask, bool droppingOnlyLaunchers,
                                           bool droppingSeparator, bool droppingFiles)
{
    return DropClassifier::tasksDragAccepts({.movingTask = movingTask,
                                             .droppingOnlyLaunchers = droppingOnlyLaunchers,
                                             .droppingSeparator = droppingSeparator,
                                             .droppingFiles = droppingFiles});
}

int DropClassifierTools::tasksDropAction(bool movingTask, bool droppingOnlyLaunchers,
                                         bool droppingSeparator, bool droppingFiles)
{
    const DropClassifier::TasksDropFlags flags{.movingTask = movingTask,
                                               .droppingOnlyLaunchers = droppingOnlyLaunchers,
                                               .droppingSeparator = droppingSeparator,
                                               .droppingFiles = droppingFiles};

    switch (DropClassifier::tasksDropAction(flags)) {
    case DropClassifier::TasksDropAction::Ignore:
        return TasksDropIgnore;
    case DropClassifier::TasksDropAction::AddSeparator:
        return TasksDropAddSeparator;
    case DropClassifier::TasksDropAction::DropUrls:
        return TasksDropUrls;
    case DropClassifier::TasksDropAction::LeaveUnchanged:
        return TasksDropLeaveUnchanged;
    }

    Q_UNREACHABLE();
}

int DropClassifierTools::separatorDropPosition(bool hasHoveredItem, int hoveredItemIndex)
{
    return DropClassifier::separatorDropPosition(hasHoveredItem ? std::optional<int>{hoveredItemIndex}
                                                                : std::nullopt);
}

namespace {

//! strict map reads: a missing or mistyped key is a shell bug, refused
//! loudly - never defaulted around (a defaulted-to-false identity flag
//! would silently re-route drags)
std::optional<bool> boolAt(const QVariantMap &map, const char *key)
{
    const QVariant value = map.value(QLatin1StringView(key));

    if (value.typeId() != QMetaType::Bool) {
        return std::nullopt;
    }

    return value.toBool();
}

//! QML numbers arrive as int or double depending on the expression;
//! anything else (bool, string, null) is a mistyped key
bool isNumeric(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Float:
    case QMetaType::Double:
        return true;
    default:
        return false;
    }
}

std::optional<qreal> realAt(const QVariantMap &map, const char *key)
{
    const QVariant value = map.value(QLatin1StringView(key));

    if (!isNumeric(value)) {
        return std::nullopt;
    }

    return value.toReal();
}

std::optional<int> intAt(const QVariantMap &map, const char *key)
{
    const QVariant value = map.value(QLatin1StringView(key));

    if (!isNumeric(value)) {
        return std::nullopt;
    }

    //! itemIndex is integral by contract; a fractional value is a
    //! mistyped key, not something to truncate quietly
    const double real = value.toDouble();
    const int integral = static_cast<int>(real);

    if (static_cast<double>(integral) != real) {
        return std::nullopt;
    }

    return integral;
}

}

QVariantMap DropClassifierTools::decideTasksDragMove(const QVariantMap &snapshot)
{
    const auto refuse = [&snapshot](const char *what) -> QVariantMap {
        qCritical() << "DropClassifier: decideTasksDragMove snapshot is malformed (" << what
                    << ")" << snapshot << "- refusing to route";
        return {{QStringLiteral("action"), int(MoveTaskNone)}};
    };

    DropClassifier::TasksDragMoveSnapshot core;

    //! absence is spelled as an explicit null; a MISSING key is a shell
    //! bug (it would silently classify every drag as external)
    if (!snapshot.contains(QStringLiteral("dragSource")) || !snapshot.contains(QStringLiteral("above"))) {
        return refuse("dragSource/above keys must be present (null spells absence)");
    }

    const QVariant dragSource = snapshot.value(QStringLiteral("dragSource"));
    if (dragSource.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = dragSource.toMap();
        const auto itemIndex = intAt(map, "itemIndex");
        const auto isLauncher = boolAt(map, "isLauncher");
        const auto isAbove = boolAt(map, "isAbove");

        if (!itemIndex || !isLauncher || !isAbove) {
            return refuse("dragSource");
        }

        core.dragSource = DropClassifier::DragSourceState{.itemIndex = *itemIndex,
                                                          .isLauncher = *isLauncher,
                                                          .isAbove = *isAbove};
    } else if (!dragSource.isNull()) {
        return refuse("dragSource");
    }

    const QVariant above = snapshot.value(QStringLiteral("above"));
    if (above.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = above.toMap();
        const auto itemIndex = intAt(map, "itemIndex");
        const auto hasModelData = boolAt(map, "hasModelData");
        const auto isLauncher = boolAt(map, "isLauncher");

        if (!itemIndex || !hasModelData || !isLauncher) {
            return refuse("above");
        }

        core.above = DropClassifier::AboveItemState{.itemIndex = *itemIndex,
                                                    .hasModelData = *hasModelData,
                                                    .isLauncher = *isLauncher};
    } else if (!above.isNull()) {
        return refuse("above");
    }

    const auto hasIgnoredItem = boolAt(snapshot, "hasIgnoredItem");
    const auto aboveIsIgnored = boolAt(snapshot, "aboveIsIgnored");
    const auto aboveIsHovered = boolAt(snapshot, "aboveIsHovered");
    const auto sortIsManual = boolAt(snapshot, "sortIsManual");
    const auto vertical = boolAt(snapshot, "vertical");
    const auto posX = realAt(snapshot, "posX");
    const auto posY = realAt(snapshot, "posY");
    const auto itemStep = realAt(snapshot, "itemStep");

    if (!hasIgnoredItem || !aboveIsIgnored || !aboveIsHovered || !sortIsManual
            || !vertical || !posX || !posY || !itemStep) {
        return refuse("scalar keys");
    }

    if (*itemStep <= 0) {
        //! metrics.totals.length is positive on any live dock; zero
        //! means the shell read a torn-down metrics object - a bug to
        //! surface, never divide by
        return refuse("itemStep must be positive");
    }

    core.hasIgnoredItem = *hasIgnoredItem;
    core.aboveIsIgnored = *aboveIsIgnored;
    core.aboveIsHovered = *aboveIsHovered;
    core.sortIsManual = *sortIsManual;
    core.positionInTarget = QPointF(*posX, *posY);
    core.orientation = *vertical ? Qt::Vertical : Qt::Horizontal;
    core.itemStep = *itemStep;

    //! the identity invariant the core asserts under tests, refused
    //! loudly here where shell input arrives
    if (!core.above && !core.hasIgnoredItem && !core.aboveIsIgnored) {
        return refuse("aboveIsIgnored must be true when above and ignoredItem are both absent");
    }
    if (core.aboveIsIgnored && core.above && !core.hasIgnoredItem) {
        return refuse("a present above cannot be the ignored item when none exists");
    }

    const DropClassifier::TasksDragMoveDecision decision = DropClassifier::decideTasksDragMove(core);

    QVariantMap answer;

    switch (decision.action) {
    case DropClassifier::TasksDragMoveAction::SuppressRepeatTarget:
        answer.insert(QStringLiteral("action"), int(MoveTaskSuppressRepeatTarget));
        break;
    case DropClassifier::TasksDragMoveAction::ReorderDragSource:
        //! the core's decision invariant: a reorder always carries its target
        Q_ASSERT(decision.moveTo.has_value());
        answer.insert(QStringLiteral("action"), int(MoveTaskReorder));
        answer.insert(QStringLiteral("moveTo"), *decision.moveTo);
        break;
    case DropClassifier::TasksDragMoveAction::KeepOrder:
        answer.insert(QStringLiteral("action"), int(MoveTaskKeepOrder));
        break;
    case DropClassifier::TasksDragMoveAction::HoverAbove:
        answer.insert(QStringLiteral("action"), int(MoveTaskHoverAbove));
        break;
    case DropClassifier::TasksDragMoveAction::ClearHover:
        answer.insert(QStringLiteral("action"), int(MoveTaskClearHover));
        break;
    case DropClassifier::TasksDragMoveAction::None:
        answer.insert(QStringLiteral("action"), int(MoveTaskNone));
        break;
    }

    return answer;
}

}
