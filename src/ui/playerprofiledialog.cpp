#include "playerprofiledialog.h"
#include "ui_playerprofiledialog.h"
#include <QFontDatabase>


PlayerProfileDialog::PlayerProfileDialog(QWidget *parent) : QDialog(parent),
                                                            ui(new Ui::PlayerProfileDialog),
                                                            settings(new QSettings("sgambe33", "wtplotter", this)) {
    ui->setupUi(this);

    const int id = QFontDatabase::addApplicationFont(":/fonts/wt_symbols.ttf");
    if (id >= 0) {
        QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty()) {
            wtSymbols = QFont(families.at(0));
        }
    }

    ui->lineupTable->setColumnCount(5);
    ui->lineupTable->setHorizontalHeaderLabels({tr("Vehicle"), tr("Rank"), tr("Arcade BR"), tr("Realistic BR"), tr("Simulator BR")});
    ui->lineupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    loadVehicleDatabase();

    connect(ui->playerProfileButton, &QPushButton::clicked, this, [this] {
        const QString url = "https://warthunder.com/en/community/searchplayers?name=" + playerId;
        QDesktopServices::openUrl(QUrl(url));
    });
}

PlayerProfileDialog::~PlayerProfileDialog() {
    delete ui;
}

void PlayerProfileDialog::loadVehicleDatabase() {
    QFile file(":/translations/vehicles.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            const QJsonArray array = doc.array();
            m_vehicleDatabase.reserve(array.size());

            for (const QJsonValue &val: array) {
                if (val.isObject()) {
                    QJsonObject obj = val.toObject();
                    if (obj.contains("identifier")) {
                        m_vehicleDatabase.insert(obj["identifier"].toString(), obj);
                    }
                }
            }
        }
        file.close();
    }
}

void PlayerProfileDialog::setPlayerData(const UiPlayerData &playerData) {
    this->playerId = playerData.userId;

    const QString username = QString(playerData.name).replace("@psn", "").replace("@live", "");
    setWindowTitle(username);

    if (!playerData.country.isEmpty()) {
        ui->countryLabel->setPixmap(QPixmap(":/icons/" + playerData.country + ".png"));
    }

    ui->usernameLabel->setText(playerData.displayName);
    ui->usernameLabel->setFont(wtSymbols);
    ui->platformLabel->setText(tr(qPrintable(playerData.platform)));

    QStringList vehicles = playerData.lineup.split(',', Qt::SkipEmptyParts);

    ui->lineupTable->setRowCount(vehicles.size());
    ui->lineupTable->setSortingEnabled(false);

    const QString lang = settings->value("language", "en").toString();

    for (int i = 0; i < vehicles.size(); ++i) {
        QString vehicleId = vehicles[i].trimmed();
        QJsonObject obj = m_vehicleDatabase.value(vehicleId);

        if (obj.isEmpty()) continue;

        QTableWidgetItem *item = new QTableWidgetItem(obj.value(lang).toString());
        item->setFont(wtSymbols);

        ui->lineupTable->setItem(i, 0, item);

        auto addNumItem = [&](int col, double val) {
            QTableWidgetItem *numItem = new QTableWidgetItem();
            numItem->setData(Qt::DisplayRole, val);
            ui->lineupTable->setItem(i, col, numItem);
        };

        addNumItem(1, obj.value("rank").toInt());
        addNumItem(2, obj.value("ab_br").toDouble());
        addNumItem(3, obj.value("rb_br").toDouble());
        addNumItem(4, obj.value("sb_br").toDouble());
    }

    ui->lineupTable->resizeColumnsToContents();
    ui->lineupTable->setSortingEnabled(true);
}
