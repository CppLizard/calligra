/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2002 - 2005 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPLATO_VIEW
#define KPLATO_VIEW

#include <koView.h>
#include "kptcontext.h"

class QListViewItem;
class QPopupMenu;
class QHBoxLayout;
class QTabWidget;
class QWidgetStack;

class KListView;
class KPrinter;
class KAction;
class KSelectAction;
class KToggleAction;

class DCOPObject;

namespace KPlato
{

class GanttView;
class PertView;
class ResourceView;
class ReportView;
class Part;
class Node;
class Project;
class Relation;
class Context;

class View : public KoView {
    Q_OBJECT

public:
    View(Part* part, QWidget* parent=0, const char* name=0);
    ~View();
    /**
     * Support zooming.
     */
    virtual void setZoom(double zoom);

    Part *getPart()const;

    Project& getProject() const;

    virtual void setupPrinter(KPrinter &printer);
    virtual void print(KPrinter &printer);

    QPopupMenu *popupMenu(const QString& name);
    void setReportGenerateMenu();

    void projectCalculate();

    virtual DCOPObject* dcopObject();
    
    void renameNode(Node *node, QString name);
   
    virtual bool setContext(Context &context);
    virtual void getContext(Context &context) const;

public slots:
    void slotUpdate(bool calculate);
    void slotEditResource();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotViewGantt();
    void slotViewGanttResources();
    void slotViewGanttTaskName();
    void slotViewGanttTaskLinks();
    void slotViewGanttProgress();
    void slotViewGanttFloat();
    void slotViewGanttCriticalTasks();
    void slotViewGanttCriticalPath();
    void slotViewPert();
    void slotViewResources();
    void slotAddTask();
    void slotAddSubTask();
    void slotAddMilestone();
    void slotProjectEdit();
    void slotDefineWBS();
    void slotGenerateWBS();
    void slotConfigure();
    void slotAddRelation(Node *par, Node *child);
    void slotModifyRelation(Relation *rel);
    void slotAddRelation(Node *par, Node *child, int linkType);
    void slotModifyRelation(Relation *rel, int linkType);
  
    void setBaselineMode(bool on);
    
    void slotExportGantt(); // testing
    
protected slots:
    void slotProjectCalendar();
    void slotProjectWorktime();
    void slotProjectCalculate();
    void slotProjectAccounts();
    void slotProjectResources();
    void slotReportDesign();
    void slotReportGenerate(int);

    void slotOpenNode();
    void slotTaskProgress();
	void slotDeleteTask();
	void slotIndentTask();
	void slotUnindentTask();
	void slotMoveTaskUp();
	void slotMoveTaskDown();

    void slotConnectNode();
	void slotChanged(QWidget *);
	void slotChanged();

    void slotAboutToShow(QWidget *widget);

#ifndef NDEBUG
    void slotPrintDebug();
    void slotPrintCalendarDebug();
#else
    static void slotPrintDebug() { };
    static void slotPrintCalendarDebug() { };
#endif

protected:
    virtual void updateReadWrite(bool readwrite);
	Node *currentTask();

private:
    GanttView *m_ganttview;
    QHBoxLayout *m_ganttlayout;
    PertView *m_pertview;
    QHBoxLayout *m_pertlayout;
	QWidgetStack *m_tab;
    ResourceView *m_resourceview;
    ReportView *m_reportview;
    QPtrList<QString> m_reportTemplateFiles;

    bool m_baselineMode;
    
    int m_viewGrp;
    int m_defaultFontSize;

    DCOPObject* m_dcop;

    // ------ Edit
    KAction *actionCut;
    KAction *actionCopy;
    KAction *actionPaste;

    KAction *actionIndentTask;
    KAction *actionUnindentTask;
    KAction *actionMoveTaskUp;
    KAction *actionMoveTaskDown;
    
    // ------ View
    KAction *actionViewGantt;
    KToggleAction *actionViewGanttResources;
    KToggleAction *actionViewGanttTaskName;
    KToggleAction *actionViewGanttTaskLinks;
    KToggleAction *actionViewGanttProgress;
    KToggleAction *actionViewGanttFloat;
    KToggleAction *actionViewGanttCriticalTasks;
    KToggleAction *actionViewGanttCriticalPath;
    KAction *actionViewPert;
    KAction *actionViewResources;

    // ------ Insert
    KAction *actionAddTask;
    KAction *actionAddSubtask;
    KAction *actionAddMilestone;
    
    // ------ Project
    KAction *actionEditMainProject;
    KAction *actionEditStandardWorktime;
    KAction *actionEditCalendar;
    KAction *actionEditAccounts;
    KAction *actionEditResources;
    KAction *actionCalculate;
    
    // ------ Reports
    KSelectAction *actionReportGenerate;
    KAction *actionFirstpage;
    KAction *actionPriorpage;
    KAction *actionNextpage;
    KAction *actionLastpage;
    
    // ------ Tools
    KAction *actionDefineWBS;
    KAction *actionGenerateWBS;
    
    // ------ Export (testing)
    KAction *actionExportGantt;
    
    // ------ Settings
    KAction *actionConfigure;

    // ------ Popup
    KAction *actionOpenNode;
    KAction *actionTaskProgress;
    KAction *actionDeleteTask;
    KAction *actionEditResource;
};

} //Kplato namespace

#endif
