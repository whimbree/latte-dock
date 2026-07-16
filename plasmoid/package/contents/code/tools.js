/*
    SPDX-FileCopyrightText: 2016 Smith AR <audoban@openmailbox.org>
    SPDX-FileCopyrightText: 2016-2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

function wheelActivateNextPrevTask(wheelDelta, eventDelta) {
    // magic number 120 for common "one click"
    // See: http://qt-project.org/doc/qt-5/qml-qtquick-wheelevent.html#angleDelta-prop
    wheelDelta += eventDelta;
    var increment = 0;
    while (wheelDelta >= 120) {
        wheelDelta -= 120;
        increment++;
    }
    while (wheelDelta <= -120) {
        wheelDelta += 120;
        increment--;
    }
    while (increment != 0) {
        activateNextPrevTask(increment < 0)
        increment += (increment < 0) ? 1 : -1;
    }

    return wheelDelta;
}

function activateTask(index, model, modifiers, task) {
    if (modifiers & Qt.ControlModifier) {
        tasksModel.requestNewInstance(index);
    } else if (task.isGroupParent) {
        task.activateNextTask();
       // if (backend.canPresentWindows()) {
        //    backend.presentWindows(model.LegacyWinIdList);
       // }
        /*} else if (groupDialog.visible) {
            groupDialog.visible = false;
        } else {
            groupDialog.visualParent = task;
            groupDialog.visible = true;
        }*/
    } else {
        if (model.IsMinimized === true) {
            tasksModel.requestToggleMinimized(index);
            tasksModel.requestActivate(index);
        } else if (model.IsActive === true) {
            tasksModel.requestToggleMinimized(index);
        } else {
            tasksModel.requestActivate(index);
        }
    }
}


//! The bar's task delegates in MODEL order. icList's contentItem holds them
//! in CREATION order plus view chrome (the same collection main.qml's
//! taskExists() and icList's childAtPos() walk, with the same
//! objectName === "TaskItem" discriminator), so order by itemIndex; a dying
//! delegate reports itemIndex -1 and drops out.
function tasksInBarOrder() {
    var children = icList.contentItem.children;
    var byIndex = [];

    for (var i = 0; i < children.length; ++i) {
        var child = children[i];
        if (child.objectName !== "TaskItem" || child.itemIndex < 0) {
            continue;
        }
        byIndex[child.itemIndex] = child;
    }

    var ordered = [];
    for (var i = 0; i < byIndex.length; ++i) {
        if (byIndex[i] !== undefined) {
            ordered.push(byIndex[i]);
        }
    }

    return ordered;
}

function activateNextPrevTask(next) {
    var taskIndexList = [];
    var activeTaskIndex = tasksModel.activeTask;

    var tasks = tasksInBarOrder();

    for (var i = 0; i < tasks.length; ++i) {
        var task = tasks[i];

        if (task.isLauncher !== true && task.isStartup !== true) {
            if (task.isGroupParent === true) {
                for (var j = 0; j < tasksModel.rowCount(task.modelIndex()); ++j) {
                    taskIndexList.push(tasksModel.makeModelIndex(task.itemIndex, j));
                }
            } else {
                taskIndexList.push(task.modelIndex());
            }
        }
    }

    if (!taskIndexList.length) {
        return;
    }

    var target = taskIndexList[0];

    for (var i = 0; i < taskIndexList.length; ++i) {
        if (taskIndexList[i] === activeTaskIndex)
        {
            if (next && i < (taskIndexList.length - 1)) {
                target = taskIndexList[i + 1];
            } else if (!next) {
                if (i) {
                    target = taskIndexList[i - 1];
                } else {
                    target = taskIndexList[taskIndexList.length - 1];
                }
            }

            break;
        }
    }

    tasksModel.requestActivate(target);
}

function insertIndexAt(above, x, y) {
    if (above && above.itemIndex) {
        return above.itemIndex;
    } else {
        var distance = root.vertical ? y : x;
        //var step = root.vertical ? LayoutManager.taskWidth() : LayoutManager.taskHeight();
        var step = appletAbilities.metrics.totals.length;
        var stripe = Math.ceil(distance / step);

        /* if (stripe === LayoutManager.calculateStripes()) {
            return tasksModel.count - 1;
        } else {
            return stripe * LayoutManager.tasksPerStripe();
        }*/

        return stripe-1;
    }
}
