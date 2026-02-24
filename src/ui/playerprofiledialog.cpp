#include "playerprofiledialog.h"
#include "ui_playerprofiledialog.h"
#include <QFontDatabase>
#include <QDesktopServices>
#include <QUrl>


PlayerProfileDialog::PlayerProfileDialog(QWidget *parent)
    : QDialog(parent),
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

    // prefer reserving capacity for the vehicle DB to avoid rehashing
    loadVehicleDatabase();

    connect(ui->playerProfileButton, &QPushButton::clicked, this, [this] {
        const QString url = QStringLiteral("https://warthunder.com/en/community/searchplayers?name=") + playerId;
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
    });
}

PlayerProfileDialog::~PlayerProfileDialog() {
    delete ui;
}

void PlayerProfileDialog::loadVehicleDatabase() {
    QFile file(":/translations/vehicles.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    const QJsonArray array = doc.array();
    // reserve capacity if supported by the container type
    m_vehicleDatabase.reserve(array.size());

    for (const QJsonValue &val: array) {
        if (!val.isObject()) continue;
        const QJsonObject obj = val.toObject();
        if (!obj.contains("identifier")) continue;
        const QString id = obj.value("identifier").toString();
        if (id.isEmpty()) continue;
        // insert a copy of the object keyed by identifier
        m_vehicleDatabase.insert(id, obj);
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

    // Prepare table for bulk update to reduce repaints
    QTableWidget *table = ui->lineupTable;
    table->setSortingEnabled(false);
    table->setUpdatesEnabled(false);
    table->clearContents();

    const int vehicleCount = vehicles.size();
    table->setRowCount(vehicleCount);

    const QString lang = settings->value("language", "en").toString();

    int row = 0;
    for (const QString &rawVehicle: vehicles) {
        const QString vehicleId = rawVehicle.trimmed();
        if (vehicleId.isEmpty()) {
            ++row;
            continue;
        }

        // Use constFind to avoid creating temporary QJsonObject copies and suppress warnings
        auto it = m_vehicleDatabase.constFind(vehicleId);
        if (it == m_vehicleDatabase.constEnd()) {
            ++row;
            continue;
        }
        const QJsonObject &obj = it.value();

        // Name cell
        QTableWidgetItem *nameItem = new QTableWidgetItem(obj.value(lang).toString());
        nameItem->setFont(wtSymbols);
        table->setItem(row, 0, nameItem);

        // Helper lambda to add numeric items (int/double) with DisplayRole to allow numeric sorting
        auto addNumItem = [&](int col, const QVariant &val) {
            QTableWidgetItem *numItem = new QTableWidgetItem();
            numItem->setData(Qt::DisplayRole, val);
            numItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            table->setItem(row, col, numItem);
        };

        addNumItem(1, obj.value("rank").toInt());
        addNumItem(2, obj.value("ab_br").toDouble());
        addNumItem(3, obj.value("rb_br").toDouble());
        addNumItem(4, obj.value("sb_br").toDouble());

        ++row;
    }

    table->resizeColumnsToContents();
    table->setSortingEnabled(true);
    table->setUpdatesEnabled(true);
}
