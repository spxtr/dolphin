// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "Common/FileUtil.h"
#include "Core/ConfigManager.h"

#include "DolphinQt2/Settings.h"
#include "DolphinQt2/Settings/PathPane.h"

PathPane::PathPane(QWidget* parent) : QWidget(parent)
{
  setWindowTitle(tr("Paths"));

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(MakeGameFolderBox());
  layout->addLayout(MakePathsLayout());

  setLayout(layout);
}

void PathPane::Browse()
{
  QString dir =
      QFileDialog::getExistingDirectory(this, tr("Select a Directory"), QDir::currentPath());
  if (!dir.isEmpty())
    Settings::Instance().AddPath(dir);
}

void PathPane::BrowseDefaultGame()
{
  auto& default_iso = SConfig::GetInstance().m_strDefaultISO;

  QString file = QFileDialog::getOpenFileName(
      this, tr("Select a Game"), QString::fromStdString(default_iso),
      tr("All GC/Wii files (*.elf *.dol *.gcm *.iso *.tgc *.wbfs *.ciso *.gcz *.wad);;"
         "All Files (*)"));
  if (!file.isEmpty())
  {
    m_game_edit->setText(file);
    default_iso = file.toStdString();
  }
}

void PathPane::BrowseWiiNAND()
{
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Select Wii NAND Root"), QString::fromStdString(SConfig::GetInstance().m_NANDPath));
  if (!dir.isEmpty())
  {
    m_nand_edit->setText(dir);
    OnNANDPathChanged();
  }
}

void PathPane::BrowseDump()
{
  auto& dump_path = SConfig::GetInstance().m_DumpPath;
  QString dir = QFileDialog::getExistingDirectory(this, tr("Select Dump Path"),
                                                  QString::fromStdString(dump_path));
  if (!dir.isEmpty())
  {
    m_dump_edit->setText(dir);
    dump_path = dir.toStdString();
  }
}

void PathPane::BrowseSDCard()
{
  QString file = QFileDialog::getOpenFileName(
      this, tr("Select a SD Card Image"),
      QString::fromStdString(SConfig::GetInstance().m_strWiiSDCardPath),
      tr("SD Card Image (*.raw);;"
         "All Files (*)"));
  if (!file.isEmpty())
  {
    m_sdcard_edit->setText(file);
    OnSDCardPathChanged();
  }
}

void PathPane::OnSDCardPathChanged()
{
  const auto sd_card_path = m_sdcard_edit->text().toStdString();

  SConfig::GetInstance().m_strWiiSDCardPath = sd_card_path;
  File::SetUserPath(F_WIISDCARD_IDX, sd_card_path);
}

void PathPane::OnNANDPathChanged()
{
  const auto nand_path = m_nand_edit->text().toStdString();

  SConfig::GetInstance().m_NANDPath = nand_path;
  File::SetUserPath(D_WIIROOT_IDX, nand_path);
}

QGroupBox* PathPane::MakeGameFolderBox()
{
  QGroupBox* game_box = new QGroupBox(tr("Game Folders"));
  QVBoxLayout* vlayout = new QVBoxLayout;

  m_path_list = new QListWidget;
  m_path_list->insertItems(0, Settings::Instance().GetPaths());
  m_path_list->setSpacing(1);
  connect(&Settings::Instance(), &Settings::PathAdded,
          [this](const QString& dir) { m_path_list->addItem(dir); });
  connect(&Settings::Instance(), &Settings::PathRemoved, [this](const QString& dir) {
    auto items = m_path_list->findItems(dir, Qt::MatchExactly);
    for (auto& item : items)
      delete item;
  });
  vlayout->addWidget(m_path_list);

  QHBoxLayout* hlayout = new QHBoxLayout;

  hlayout->addStretch();
  QPushButton* add = new QPushButton(tr("Add"));
  QPushButton* remove = new QPushButton(tr("Remove"));

  auto* checkbox = new QCheckBox(tr("Search Subfolders"));
  checkbox->setChecked(SConfig::GetInstance().m_RecursiveISOFolder);

  hlayout->addWidget(add);
  hlayout->addWidget(remove);
  vlayout->addLayout(hlayout);
  vlayout->addWidget(checkbox);

  connect(checkbox, &QCheckBox::toggled, this, [this](bool checked) {
    SConfig::GetInstance().m_RecursiveISOFolder = checked;
    for (const auto& path : Settings::Instance().GetPaths())
      Settings::Instance().ReloadPath(path);
  });

  connect(add, &QPushButton::clicked, this, &PathPane::Browse);
  connect(remove, &QPushButton::clicked, this, &PathPane::RemovePath);

  game_box->setLayout(vlayout);
  return game_box;
}

QGridLayout* PathPane::MakePathsLayout()
{
  QGridLayout* layout = new QGridLayout;
  layout->setColumnStretch(1, 1);

  m_game_edit = new QLineEdit(Settings::Instance().GetDefaultGame());
  connect(m_game_edit, &QLineEdit::editingFinished,
          [this] { Settings::Instance().SetDefaultGame(m_game_edit->text()); });
  connect(&Settings::Instance(), &Settings::DefaultGameChanged,
          [this](const QString& path) { m_game_edit->setText(path); });
  QPushButton* game_open = new QPushButton(QStringLiteral("..."));
  connect(game_open, &QPushButton::clicked, this, &PathPane::BrowseDefaultGame);
  layout->addWidget(new QLabel(tr("Default ISO:")), 0, 0);
  layout->addWidget(m_game_edit, 0, 1);
  layout->addWidget(game_open, 0, 2);

  m_nand_edit = new QLineEdit(QString::fromStdString(SConfig::GetInstance().m_NANDPath));
  connect(m_nand_edit, &QLineEdit::editingFinished, this, &PathPane::OnNANDPathChanged);
  QPushButton* nand_open = new QPushButton(QStringLiteral("..."));
  connect(nand_open, &QPushButton::clicked, this, &PathPane::BrowseWiiNAND);
  layout->addWidget(new QLabel(tr("Wii NAND Root:")), 1, 0);
  layout->addWidget(m_nand_edit, 1, 1);
  layout->addWidget(nand_open, 1, 2);

  m_dump_edit = new QLineEdit(QString::fromStdString(SConfig::GetInstance().m_DumpPath));
  connect(m_dump_edit, &QLineEdit::editingFinished,
          [=] { SConfig::GetInstance().m_DumpPath = m_dump_edit->text().toStdString(); });
  QPushButton* dump_open = new QPushButton(QStringLiteral("..."));
  connect(dump_open, &QPushButton::clicked, this, &PathPane::BrowseDump);
  layout->addWidget(new QLabel(tr("Dump Path:")), 2, 0);
  layout->addWidget(m_dump_edit, 2, 1);
  layout->addWidget(dump_open, 2, 2);

  m_sdcard_edit = new QLineEdit(QString::fromStdString(SConfig::GetInstance().m_strWiiSDCardPath));
  connect(m_sdcard_edit, &QLineEdit::editingFinished, this, &PathPane::OnSDCardPathChanged);
  QPushButton* sdcard_open = new QPushButton(QStringLiteral("..."));
  connect(sdcard_open, &QPushButton::clicked, this, &PathPane::BrowseSDCard);
  layout->addWidget(new QLabel(tr("SD Card Path:")), 3, 0);
  layout->addWidget(m_sdcard_edit, 3, 1);
  layout->addWidget(sdcard_open, 3, 2);

  return layout;
}

void PathPane::RemovePath()
{
  auto item = m_path_list->currentItem();
  if (!item)
    return;
  Settings::Instance().RemovePath(item->text());
}
