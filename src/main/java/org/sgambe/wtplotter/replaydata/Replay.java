package org.sgambe.wtplotter.replaydata;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

public class Replay {

    private static final byte[] MAGIC = new byte[]{(byte) 0xe5, (byte) 0xac, 0x00, 0x10};
    private final int version;
    private final String level;
    private final String levelSettings;
    private final String battleType;
    private final String environment;
    private final String visibility;
    private final int rezOffset;
    private final byte difficulty;
    private final byte sessionType;
    private final String sessionId;
    private final int mSetSize;
    private final String locName;
    private final int startTime;
    private final int timeLimit;
    private final int scoreLimit;
    private final String battleClass;
    private final String battleKillStreak;
    private String status;
    private double timePlayed;
    private String authorUserId;
    private String author;
    private List<Player> players; // List of players
    private List<PlayerInfo> playersInfo; // List of detailed player info


    public Replay(ByteBuffer buffer) {
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        byte[] magic = new byte[4];
        buffer.get(magic);
        if (!java.util.Arrays.equals(magic, MAGIC)) {
            throw new IllegalArgumentException("Invalid magic number");
        }

        this.version = buffer.getInt();
        this.level = readString(buffer, 128).replace("levels/", "").replace(".bin", "");
        this.levelSettings = readString(buffer, 260);
        this.battleType = readString(buffer, 128);
        this.environment = readString(buffer, 128);
        this.visibility = readString(buffer, 32);
        this.rezOffset = buffer.getInt();
        this.difficulty = buffer.get();
        buffer.position(buffer.position() + 35); // Skip 35 bytes
        this.sessionType = buffer.get();
        buffer.position(buffer.position() + 7); // Skip 7 bytes
        this.sessionId = Long.toHexString(buffer.getLong());

        buffer.position(buffer.position() + 4); // Skip 4 bytes
        this.mSetSize = buffer.getInt();
        buffer.position(buffer.position() + 32); // Skip 28 bytes
        this.locName = readString(buffer, 128);
        this.startTime = buffer.getInt();
        this.timeLimit = buffer.getInt();
        this.scoreLimit = buffer.getInt();
        buffer.position(buffer.position() + 48); // Skip 48 bytes
        this.battleClass = readString(buffer, 128);
        this.battleKillStreak = readString(buffer, 128);
        try {
            JsonObject results = unpackResults(rezOffset, buffer);
            parseResults(results);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public static Replay fromFile(File replayFile) throws IOException {
        byte[] replayFileBytes = Files.readAllBytes(replayFile.toPath());
        return new Replay(ByteBuffer.wrap(replayFileBytes));
    }

    private String readString(ByteBuffer buffer, int length) {
        byte[] bytes = new byte[length];
        buffer.get(bytes);
        int nullIndex = 0;
        while (nullIndex < bytes.length && bytes[nullIndex] != 0) {
            nullIndex++;
        }
        return new String(bytes, 0, nullIndex, StandardCharsets.UTF_8);
    }

    private JsonObject unpackResults(int rezOffset, ByteBuffer buffer) throws IOException {
        byte[] bytesAfterRezOffset = new byte[buffer.array().length - rezOffset];
        System.arraycopy(buffer.array(), rezOffset, bytesAfterRezOffset, 0, bytesAfterRezOffset.length);
        ProcessBuilder processBuilder = new ProcessBuilder(System.getProperty("java.io.tmpdir") + "/wtplotter/wt_ext_cli.exe", "--unpack_raw_blk", "--stdout", "--stdin", "--format", "Json");
        Process process = processBuilder.start();

        try (OutputStream os = process.getOutputStream()) {
            os.write(bytesAfterRezOffset);
            os.flush();
        }

        StringBuilder jsonOutput = new StringBuilder();
        try (InputStream is = process.getInputStream(); BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
            String line;
            while ((line = reader.readLine()) != null) {
                jsonOutput.append(line);
            }
        }
        return JsonParser.parseString(jsonOutput.toString()).getAsJsonObject();
    }

    private void parseResults(JsonObject results) {
        if (results.has("status")) {
            this.status = results.get("status").getAsString();
        } else {
            this.status = "Left the game";
        }
        this.timePlayed = results.get("timePlayed").getAsDouble();
        this.authorUserId = results.get("authorUserId").getAsString();
        this.author = results.get("author").getAsString();
        JsonArray players = results.get("player").getAsJsonArray();
        this.players = parsePlayers(players);
        JsonObject uiScriptsData = results.get("uiScriptsData").getAsJsonObject();
        JsonObject playersInfo = uiScriptsData.get("playersInfo").getAsJsonObject();
        this.playersInfo = parsePlayersInfo(playersInfo);
    }

    private List<Player> parsePlayers(JsonArray players) {
        List<Player> playerList = new ArrayList<>();
        players.forEach(jsonElement -> {
            JsonObject playerObject = jsonElement.getAsJsonObject();
            playerList.add(new Player().fromJson(playerObject));
        });
        return playerList;
    }

    private List<PlayerInfo> parsePlayersInfo(JsonObject playersInfo) {
        List<PlayerInfo> playerInfoList = new ArrayList<>();
        playersInfo.asMap().forEach((key, value) -> {
            JsonObject playerInfoObject = value.getAsJsonObject();
            playerInfoList.add(new PlayerInfo().fromJson(playerInfoObject));
        });
        return playerInfoList;
    }


    @Override
    public String toString() {
        return "ReplayHeader{" + "version=" + version + ", level='" + level + '\'' + ", levelSettings='" + levelSettings + '\'' + ", battleType='" + battleType + '\'' + ", environment='" + environment + '\'' + ", visibility='" + visibility + '\'' + ", rezOffset=" + rezOffset + ", difficulty=" + difficulty + ", sessionType=" + sessionType + ", sessionId=" + sessionId + ", mSetSize=" + mSetSize + ", locName='" + locName + '\'' + ", startTime=" + startTime + ", timeLimit=" + timeLimit + ", scoreLimit=" + scoreLimit + ", battleClass='" + battleClass + '\'' + ", battleKillStreak='" + battleKillStreak + '\'' + '}';
    }

    public String getLevel() {
        return level;
    }

    public int getStartTime() {
        return startTime;
    }

    public double getTimePlayed() {
        return timePlayed;
    }

    public String getStatus() {
        return status;
    }

    public String getLevelSettings() {
        return levelSettings;
    }

    public String getBattleType() {
        return battleType;
    }

    public List<Player> getPlayers() {
        return players;
    }

    public PlayerInfo getPlayerInfoByUsername(String username) {
        for (PlayerInfo playerInfo : playersInfo) {
            if (playerInfo.getName().equals(username)) {
                return playerInfo;
            }
        }
        return null;
    }

    public String getSessionId() {
        return sessionId;
    }

    public String getAuthorUserId() {
        return authorUserId;
    }
}
