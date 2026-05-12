/** Copyright (C) 2010, J.M.Reneau.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include <QObject>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QAction>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QToolTip>
#include <QFileIconProvider>
#include <QTreeWidget>

#ifndef PREFERENCESWINH

#define PREFERENCESWINH


class SettingsBrowser : public QDialog
{
  Q_OBJECT

public:
    SettingsBrowser(QWidget * parent);
    //void closeEvent(QCloseEvent *);

private slots:
    void treeWidgetCheckboxChanged();
    void clickDeleteButton();

private:
    QTreeWidget *treeWidgetSettings;
    QPushButton *deleteselectedsettings;
    QPushButton *canceldeletesettings;
};


class PreferencesWin : public QDialog
{
  Q_OBJECT

public:
    PreferencesWin(QWidget * parent, bool );
	void closeEvent(QCloseEvent *);

private slots:
	void clickCancelButton();
	void clickSaveButton();
    void setDigitsValue(int);
    void setDebugSpeedValue(int);
    void setVolumeValue(int);
    void setNormalizeValue(int);
    void setVolumeRestoreValue(int);
    void clickClearSavedData();
    void clickBrowseSavedData();
  
private:
    QPushButton *clearsaveddata;
    QPushButton *browsesaveddata;

	// advanced Tab
	QWidget * advancedtabwidget;
	QGridLayout * advancedtablayout;
	QLabel *passwordlabel;
	QLineEdit *passwordinput;
	QCheckBox *settingcheckbox;

    QGroupBox *settingsgroup;
    QRadioButton *settingsacces0;
    QRadioButton *settingsacces1;
    QRadioButton *settingsacces2;

    QLabel *allowsystemlabel;
    QComboBox *allowsystemcombo;
    QLabel *allowportlabel;
    QComboBox *allowportcombo;
    QLabel *settingsaccesslabel;
    QComboBox *settingsaccesscombo;
    QLabel *settingsmaxlabel;
    QComboBox *settingsmaxcombo;

    SettingsBrowser *settingsbrowser;

	// user tab
	QWidget * usertabwidget;
	QGridLayout * usertablayout;
	QCheckBox *saveonruncheckbox;
	QLabel *typeconvlabel;
	QComboBox *typeconvcombo;
	QLabel *varnotassignedlabel;
	QComboBox *varnotassignedcombo;
	QLabel *decdigslabel;
    QLabel *decdigsvalue;
	QSlider *decdigsslider;
    QCheckBox *floattailcheckbox;
    QCheckBox *floatlocalecheckbox;
    QLabel *debugspeedlabel;
    QLabel *debugspeedvalue;
	QSlider *debugspeedslider;
    QCheckBox *windowsrestorecheckbox;
    QLabel *startuplabel;
    QCheckBox *checkupdatecheckbox;

	// sound tab
	QWidget * soundtabwidget;
	QGridLayout * soundtablayout;
	QLabel *voicelabel;
	QComboBox *voicecombo;
	QLabel *speakstmtlabel;
	QLineEdit *speakstmtinput;
   QLabel *soundvolumelabel;
    QLabel *soundvolumevalue;
    QSlider *soundvolumeslider;
    QLabel *soundnormalizelabel;
    QLabel *soundnormalizevalue;
    QSlider *soundnormalizeslider;
    QLabel *soundvolumerestorelabel;
    QLabel *soundvolumerestorevalue;
    QSlider *soundvolumerestoreslider;
    QLabel *soundsampleratelabel;
    QComboBox *soundsampleratecombo;

	// printer tab
	QWidget * printertabwidget;
	QGridLayout * printertablayout;
	QGroupBox *resolutiongroup;
	QRadioButton *resolutionhigh;
	QRadioButton *resolutionscreen;
	QGroupBox *orientgroup;
	QRadioButton *orientportrait;
	QRadioButton *orientlandscape;
	QLabel *printerslabel;
	QComboBox *printerscombo;
	QLabel *pdffilelabel;
	QLineEdit *pdffileinput;
	QLabel *paperlabel;
	QComboBox *papercombo;



	QPushButton *cancelbutton;
	QPushButton *savebutton;

};

#endif
