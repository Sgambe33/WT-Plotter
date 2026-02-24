#include "dbmanager.h"
#include <QSqlError>
#include <QDebug>
#include <nlohmann/json.hpp>

#include "logger.h"
#include "libs/WRPL_parser/include/ReplayStructs.h"
#include "libs/WRPL_parser/include/BLK.h"

using json = nlohmann::json;

namespace {
    void populateReplayHeaderFromQuery(const QSqlQuery &query, ReplayHeader &header) {
        header.sessionId = query.value("session_id").toString().toStdString();
        header.wrplVersion = query.value("wrpl_version").toUInt();
        header.rawLevel = query.value("raw_level").toString().toStdString();
        header.rawLevelSettings = query.value("raw_level_settings").toString().toStdString();
        header.rawBattleType = query.value("raw_battle_type").toString().toStdString();
        header.rawEnvironment = query.value("raw_environment").toString().toStdString();
        header.rawVisibility = query.value("raw_visibility").toString().toStdString();
        header.difficulty = query.value("difficulty").toInt();
        header.sessionType = query.value("session_type").toUInt();
        header.isServer = query.value("is_server").toInt() != 0;
        header.rawLocationName = query.value("raw_location_name").toString().toStdString();
        header.startTimeEpochS = query.value("start_time_epoch_ms").toULongLong();
        header.timeLimitInMinutes = query.value("time_limit_in_minutes").toUInt();
        header.scoreLimit = query.value("score_limit").toUInt();
        header.rawBattleClass = query.value("raw_battle_class").toString().toStdString();
        header.rawBattleKillStreak = query.value("raw_battle_kill_streak").toString().toStdString();
    }

    template<typename T>
    QByteArray serializePacketVector(const std::vector<T> &packets) {
        QByteArray blob;
        QDataStream stream(&blob, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_0);

        stream << static_cast<quint32>(packets.size());

        if constexpr (std::is_same_v<T, ChatPacket>) {
            for (const auto &p: packets) {
                stream << static_cast<quint32>(p.time);
                stream << QString::fromStdString(p.sender);
                stream << static_cast<quint32>(p.channel);
                stream << p.isEnemy;
                stream << QString::fromStdString(p.message);
            }
        } else if constexpr (std::is_same_v<T, KillPacket>) {
            for (const auto &p: packets) {
                stream << static_cast<quint32>(p.time);
                stream << static_cast<quint32>(p.control);
                stream << static_cast<quint32>(p.damageType);
                stream << static_cast<quint32>(p.killerId);
                stream << QString::fromStdString(p.killerVehicle);
            }
        } else if constexpr (std::is_same_v<T, AwardPacket>) {
            for (const auto &p: packets) {
                stream << static_cast<quint32>(p.time);
                stream << static_cast<quint32>(p.awardType);
                stream << static_cast<quint32>(p.playerId);
                stream << QString::fromStdString(p.awardName);
            }
        } else if constexpr (std::is_same_v<T, MovementPacket>) {
            for (const auto &p: packets) {
                stream << static_cast<quint32>(p.time);
                stream << static_cast<quint64>(p.entityId);
                stream << p.x;
                stream << p.y;
                stream << p.z;
            }
        } else if constexpr (std::is_same_v<T, SlotPacket>) {
            for (const auto &p: packets) {
                stream << static_cast<quint32>(p.time);
                stream << static_cast<quint32>(p.players.size());
                for (const auto &[slot, player]: p.players) {
                    stream << slot;
                    stream << static_cast<quint32>(player.userId);
                    stream << QString::fromStdString(player.name);
                    stream << QString::fromStdString(player.clanTag);
                    stream << QString::fromStdString(player.title);
                }
            }
        }
        return qCompress(blob, -1);
    }

    template<typename T>
    std::vector<T> deserializePacketVector(const QByteArray &compressedBlob) {
        std::vector<T> packets;

        // 1. Handle empty blobs gracefully
        if (compressedBlob.isEmpty()) {
            return packets;
        }

        // 2. Decompress the Qt-compressed binary blob
        QByteArray blob = qUncompress(compressedBlob);
        if (blob.isEmpty()) {
            qWarning() << "Failed to decompress packet vector!";
            return packets;
        }

        // 3. Set up the data stream for reading
        QDataStream stream(&blob, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0);

        // 4. Read the total number of packets
        quint32 count;
        stream >> count;
        packets.reserve(count); // Pre-allocate memory for performance

        // 5. Read the specific fields based on the struct type
        if constexpr (std::is_same_v<T, ChatPacket>) {
            for (quint32 i = 0; i < count; ++i) {
                ChatPacket p;
                quint32 time, channel;
                QString sender, message;

                stream >> time >> sender >> channel >> p.isEnemy >> message;

                p.time = time;
                p.sender = sender.toStdString();
                p.channel = channel;
                p.message = message.toStdString();
                packets.push_back(std::move(p));
            }
        } else if constexpr (std::is_same_v<T, KillPacket>) {
            for (quint32 i = 0; i < count; ++i) {
                KillPacket p;
                quint32 time, control, damageType, killerId;
                QString killerVehicle;

                stream >> time >> control >> damageType >> killerId >> killerVehicle;

                p.time = time;
                p.control = control;
                p.damageType = damageType;
                p.killerId = killerId;
                p.killerVehicle = killerVehicle.toStdString();
                packets.push_back(std::move(p));
            }
        } else if constexpr (std::is_same_v<T, AwardPacket>) {
            for (quint32 i = 0; i < count; ++i) {
                AwardPacket p;
                quint32 time, awardType, playerId;
                QString awardName;

                stream >> time >> awardType >> playerId >> awardName;

                p.time = time;
                p.awardType = awardType;
                p.playerId = playerId;
                p.awardName = awardName.toStdString();
                packets.push_back(std::move(p));
            }
        } else if constexpr (std::is_same_v<T, MovementPacket>) {
            for (quint32 i = 0; i < count; ++i) {
                MovementPacket p;
                quint32 time;
                quint64 entityId;

                stream >> time >> entityId >> p.x >> p.y >> p.z;

                p.time = time;
                p.entityId = entityId;
                packets.push_back(std::move(p));
            }
        } else if constexpr (std::is_same_v<T, SlotPacket>) {
            for (quint32 i = 0; i < count; ++i) {
                SlotPacket p;
                quint32 time, playersSize;

                stream >> time >> playersSize;
                p.time = time;
                p.players.reserve(playersSize);

                for (quint32 j = 0; j < playersSize; ++j) {
                    uint8_t slot;
                    quint32 userId;
                    QString name, clanTag, title;

                    stream >> slot >> userId >> name >> clanTag >> title;

                    Player player;
                    player.userId = userId;
                    player.name = name.toStdString();
                    player.clanTag = clanTag.toStdString();
                    player.title = title.toStdString();

                    p.players.push_back({slot, std::move(player)});
                }
                packets.push_back(std::move(p));
            }
        }
        return packets;
    }

    BlkMap deserializeJsonToBlkMap(const QString &jsonString) {
        BlkMap map;

        if (jsonString.isEmpty() || jsonString == "{}") {
            return map;
        }

        try {
            nlohmann::json j = nlohmann::json::parse(jsonString.toStdString());
            map = j.get<BlkMap>(); //TODO: add from_json in BLK.h!
        } catch (const nlohmann::json::exception &e) {
            qWarning() << "Failed to parse BLK JSON from database:" << e.what();
        }

        return map;
    }
}

DbManager::DbManager(const QString &path, const QString &connName, QObject *parent)
    : QObject(parent), m_db(QSqlDatabase::addDatabase("QSQLITE", connName)) {
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qCritical() << "Database connection error:" << m_db.lastError().text();
    } else {
        qInfo() << "Database connected successfully";
        prepareQueries();
        createTables();
    }
}

DbManager::~DbManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void DbManager::prepareQueries() {
    // ReplayMetadata table insert
    m_insertReplayMetadataQuery = QSqlQuery(m_db);
    m_insertReplayMetadataQuery.prepare(R"(
        INSERT OR IGNORE INTO ReplayMetadata
        (session_id, author_id, wrpl_version, raw_level, raw_level_settings, raw_battle_type,
         raw_environment, raw_visibility, difficulty, session_type, is_server, raw_location_name,
         start_time_epoch_ms, time_limit_in_minutes, score_limit, raw_battle_class, raw_battle_kill_streak)
        VALUES
        (:session_id, :author_id, :wrpl_version, :raw_level, :raw_level_settings, :raw_battle_type,
         :raw_environment, :raw_visibility, :difficulty, :session_type, :is_server, :raw_location_name,
         :start_time_epoch_ms, :time_limit_in_minutes, :score_limit, :raw_battle_class, :raw_battle_kill_streak)
    )");

    // ReplayData table insert
    m_insertReplayDataQuery = QSqlQuery(m_db);
    m_insertReplayDataQuery.prepare(R"(
        INSERT OR REPLACE INTO ReplayData
        (session_id, settings_blk, results_blk, chat_packets, kill_packets, award_packets, movement_packets, slot_packets)
        VALUES
        (:session_id, :settings_blk, :results_blk, :chat_packets, :kill_packets, :award_packets, :movement_packets, :slot_packets)
    )");
}

void DbManager::createTables() const {
    QSqlQuery query(m_db);
    query.exec("PRAGMA journal_mode = WAL");
    query.exec("PRAGMA synchronous = NORMAL");

    const QStringList tableDefinitions = {
        R"(
            CREATE TABLE IF NOT EXISTS ReplayMetadata (
                session_id TEXT PRIMARY KEY,
                author_id INTEGER,
                wrpl_version INTEGER,                 -- ReplayHeader.wrplVersion
                raw_level TEXT,                       -- ReplayHeader.rawLevel
                raw_level_settings TEXT,              -- ReplayHeader.rawLevelSettings
                raw_battle_type TEXT,                 -- ReplayHeader.rawBattleType
                raw_environment TEXT,                 -- ReplayHeader.rawEnvironment
                raw_visibility TEXT,                  -- ReplayHeader.rawVisibility
                difficulty INTEGER,                   -- ReplayHeader.difficulty
                session_type INTEGER,                 -- ReplayHeader.sessionType
                is_server INTEGER,                    -- ReplayHeader.isServer (SQLite uses INTEGER 0/1 for booleans)
                raw_location_name TEXT,               -- ReplayHeader.rawLocationName
                start_time_epoch_ms INTEGER,          -- ReplayHeader.startTimeEpochS
                time_limit_in_minutes INTEGER,        -- ReplayHeader.timeLimitInMinutes
                score_limit INTEGER,                  -- ReplayHeader.scoreLimit
                raw_battle_class TEXT,                -- ReplayHeader.rawBattleClass
                raw_battle_kill_streak TEXT           -- ReplayHeader.rawBattleKillStreak
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS ReplayData (
                session_id TEXT PRIMARY KEY,        -- Foreign key to replays.id
                settings_blk JSON,
                results_blk JSON,
                chat_packets BLOB,                    -- Compressed serialized vector<ChatPacket>
                kill_packets BLOB,                    -- Compressed serialized vector<KillPacket>
                award_packets BLOB,                   -- Compressed serialized vector<AwardPacket>
                movement_packets BLOB,                -- Compressed serialized vector<MovementPacket>
                slot_packets BLOB,                    -- Compressed serialized vector<SlotPacket>
                FOREIGN KEY (session_id) REFERENCES ReplayMetadata(session_id) ON DELETE CASCADE
            )
        )"

    };

    for (const QString &tableSql: tableDefinitions) {
        if (!query.exec(tableSql)) {
            qCritical() << "Table creation failed:" << query.lastError().text();
        }
    }
}

bool DbManager::insertReplay(const wrpl::Replay &replay) {
    if (!m_db.transaction()) {
        qCritical() << "Transaction start failed:" << m_db.lastError().text();
        return false;
    }

    try {
        m_insertReplayMetadataQuery.bindValue(":session_id", QString::fromStdString(replay.header.sessionId));
        m_insertReplayMetadataQuery.bindValue(":author_id", 0); // TODO: extract from results if available
        m_insertReplayMetadataQuery.bindValue(":wrpl_version", replay.header.wrplVersion);
        m_insertReplayMetadataQuery.bindValue(":raw_level", QString::fromStdString(replay.header.rawLevel));
        m_insertReplayMetadataQuery.bindValue(":raw_level_settings", QString::fromStdString(replay.header.rawLevelSettings));
        m_insertReplayMetadataQuery.bindValue(":raw_battle_type", QString::fromStdString(replay.header.rawBattleType));
        m_insertReplayMetadataQuery.bindValue(":raw_environment", QString::fromStdString(replay.header.rawEnvironment));
        m_insertReplayMetadataQuery.bindValue(":raw_visibility", QString::fromStdString(replay.header.rawVisibility));
        m_insertReplayMetadataQuery.bindValue(":difficulty", replay.header.difficulty);
        m_insertReplayMetadataQuery.bindValue(":session_type", replay.header.sessionType);
        m_insertReplayMetadataQuery.bindValue(":is_server", replay.header.isServer ? 1 : 0);
        m_insertReplayMetadataQuery.bindValue(":raw_location_name", QString::fromStdString(replay.header.rawLocationName));
        m_insertReplayMetadataQuery.bindValue(":start_time_epoch_ms", replay.header.startTimeEpochS);
        m_insertReplayMetadataQuery.bindValue(":time_limit_in_minutes", replay.header.timeLimitInMinutes);
        m_insertReplayMetadataQuery.bindValue(":score_limit", replay.header.scoreLimit);
        m_insertReplayMetadataQuery.bindValue(":raw_battle_class", QString::fromStdString(replay.header.rawBattleClass));
        m_insertReplayMetadataQuery.bindValue(":raw_battle_kill_streak", QString::fromStdString(replay.header.rawBattleKillStreak));

        if (!m_insertReplayMetadataQuery.exec()) {
            qCritical() << "Replay insert failed:" << m_insertReplayMetadataQuery.lastError().text();
            throw std::runtime_error("Replay insert failed");
        }

        nlohmann::json settingsJson = replay.settings;
        nlohmann::json resultsJson = replay.results;

        // Insert ReplayData (packets)
        m_insertReplayDataQuery.bindValue(":session_id", QString::fromStdString(replay.header.sessionId));
        m_insertReplayDataQuery.bindValue(":settings_blk", QString::fromStdString(settingsJson.dump()));
        m_insertReplayDataQuery.bindValue(":results_blk", QString::fromStdString(resultsJson.dump()));

        // Serialize packets to binary BLOB
        m_insertReplayDataQuery.bindValue(":chat_packets", serializePacketVector(replay.chatPackets));
        m_insertReplayDataQuery.bindValue(":kill_packets", serializePacketVector(replay.killPackets));
        m_insertReplayDataQuery.bindValue(":award_packets", serializePacketVector(replay.awardPackets));
        m_insertReplayDataQuery.bindValue(":movement_packets", serializePacketVector(replay.movementPackets));
        m_insertReplayDataQuery.bindValue(":slot_packets", serializePacketVector(replay.slotPackets));

        if (!m_insertReplayDataQuery.exec()) {
            qCritical() << "ReplayData insert failed:" << m_insertReplayDataQuery.lastError().text();
            throw std::runtime_error("ReplayData insert failed");
        }

        if (!m_db.commit()) {
            throw std::runtime_error("Commit failed");
        }

        return true;
    } catch (const std::exception &e) {
        m_db.rollback();
        qCritical() << "Database error:" << e.what();
        return false;
    }
}

QMap<QDate, QList<wrpl::Replay> > DbManager::fetchReplaysGroupedByDate() const {
    QMap<QDate, QList<wrpl::Replay> > replayMap;
    QSqlQuery query(m_db);

    query.prepare(R"(
        SELECT session_id, author_id, wrpl_version, raw_level, raw_level_settings, raw_battle_type,
               raw_environment, raw_visibility, difficulty, session_type, is_server, raw_location_name,
               start_time_epoch_ms, time_limit_in_minutes, score_limit, raw_battle_class, raw_battle_kill_streak
        FROM ReplayMetadata
        ORDER BY start_time_epoch_ms DESC
    )");

    if (!query.exec()) {
        qWarning() << "Failed to fetch replays:" << query.lastError().text();
        return replayMap;
    }

    while (query.next()) {
        wrpl::Replay replay;
        populateReplayHeaderFromQuery(query, replay.header);

        QDate dateKey = QDateTime::fromSecsSinceEpoch(replay.header.startTimeEpochS).date();
        replayMap[dateKey].append(replay);
    }

    for (auto &replays: replayMap) {
        std::sort(replays.begin(), replays.end(), [](const wrpl::Replay &a, const wrpl::Replay &b) {
            return a.header.startTimeEpochS > b.header.startTimeEpochS;
        });
    }

    return replayMap;
}

quint32 DbManager::getLatestReplay() {
    QSqlQuery query(m_db);
    if (query.exec("SELECT start_time_epoch_ms FROM ReplayMetadata ORDER BY start_time_epoch_ms DESC LIMIT 1")) {
        if (query.next()) {
            return query.value(0).toUInt();
        }
    } else {
        LOG_WARN("Failed to fetch latest replay:" + query.lastError().text());
    }
    return 0;
}

wrpl::Replay DbManager::getReplayBySessionId(const QString &sessionId) const {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT session_id, author_id, wrpl_version, raw_level, raw_level_settings, raw_battle_type,
               raw_environment, raw_visibility, difficulty, session_type, is_server, raw_location_name,
               start_time_epoch_ms, time_limit_in_minutes, score_limit, raw_battle_class, raw_battle_kill_streak
        FROM ReplayMetadata
        WHERE session_id = :session_id
    )");
    query.bindValue(":session_id", sessionId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch replay with session_id:" << sessionId << query.lastError().text();
        return wrpl::Replay();
    }

    wrpl::Replay replay;

    populateReplayHeaderFromQuery(query, replay.header);

    query.prepare(R"(
        SELECT settings_blk, results_blk, chat_packets, kill_packets, award_packets, movement_packets, slot_packets
        FROM ReplayData
        WHERE session_id = :session_id
    )");
    query.bindValue(":session_id", sessionId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to fetch replaydata with session_id:" << sessionId << query.lastError().text();
        return replay;
    }

    replay.results = deserializeJsonToBlkMap(query.value("results_blk").toString());
    replay.settings = deserializeJsonToBlkMap(query.value("settings_blk").toString());
    replay.chatPackets = deserializePacketVector<ChatPacket>(query.value("chat_packets").toByteArray());
    replay.killPackets = deserializePacketVector<KillPacket>(query.value("kill_packets").toByteArray());
    replay.awardPackets = deserializePacketVector<AwardPacket>(query.value("award_packets").toByteArray());
    replay.movementPackets = deserializePacketVector<MovementPacket>(query.value("movement_packets").toByteArray());
    replay.slotPackets = deserializePacketVector<SlotPacket>(query.value("slot_packets").toByteArray());

    qDebug() << "Deserialized" << replay.chatPackets.size() << "chat packets, "
             << replay.killPackets.size() << "kill packets, "
             << replay.awardPackets.size() << "award packets, "
             << replay.movementPackets.size() << "movement packets, "
             << replay.slotPackets.size() << "slot packets for session_id:" << sessionId;

    return replay;
}

bool DbManager::deleteReplayBySessionId(const QString &sessionId) const {
    QSqlQuery query(m_db);
    query.prepare(R"(DELETE FROM ReplayMetadata WHERE session_id = :session_id)");
    query.bindValue(":session_id", sessionId);
    if (!query.exec()) {
        qWarning() << "Failed to delete replay with session_id:" << sessionId << query.lastError().text();
        return false;
    }

    query.prepare(R"(DELETE FROM ReplayData WHERE session_id = :session_id)");
    query.bindValue(":session_id", sessionId);
    query.exec();
    return true;
}
