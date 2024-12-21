package org.sgambe.wtplotter.replaydata;

import com.google.gson.JsonObject;

import java.util.ArrayList;
import java.util.List;

public class PlayerInfo {
    private String name;
    private int team;
    private String clanTag;
    private String platform;
    private String id; // Use String for IDs to match the JSON format
    private int slot;
    private String country;
    private int mrank;
    private boolean autoSquad;
    private int waitTime;
    private String clanId;
    private int tier;
    private int squad;
    private int rank;
    private List<CraftInfo> craftsInfo; // Nested craft information

    public PlayerInfo fromJson(JsonObject playerInfoObject) {
        this.name = playerInfoObject.get("name").getAsString();
        this.team = playerInfoObject.get("team").getAsInt();
        this.clanTag = playerInfoObject.get("clanTag").getAsString();
        this.platform = playerInfoObject.get("platform").getAsString();
        this.id = playerInfoObject.get("id").getAsString();
        this.slot = playerInfoObject.get("slot").getAsInt();
        this.country = playerInfoObject.get("country").getAsString();
        this.mrank = playerInfoObject.get("mrank").getAsInt();
        this.autoSquad = playerInfoObject.get("auto_squad").getAsBoolean();
        this.waitTime = playerInfoObject.get("wait_time").getAsInt();
        this.clanId = playerInfoObject.get("clanId").getAsString();
        this.tier = playerInfoObject.get("tier").getAsInt();
        this.squad = playerInfoObject.get("squad").getAsInt();
        this.rank = playerInfoObject.get("rank").getAsInt();
        this.craftsInfo = parseCraftsInfo(playerInfoObject.getAsJsonObject("crafts_info"));
        return this;
    }

    private List<CraftInfo> parseCraftsInfo(JsonObject craftsInfoObject) {
        List<CraftInfo> craftsInfo = new ArrayList<>();
        craftsInfoObject.asMap().forEach((key, value) -> {
            if (key.equalsIgnoreCase("__array")) {
                return;
            }
            craftsInfo.add(new CraftInfo().fromJson(value.getAsJsonObject()));
        });
        return craftsInfo;
    }

    @Override
    public String toString(){
        return "PlayerInfo{" +
                "name='" + name + '\'' +
                ", team=" + team +
                ", clanTag='" + clanTag + '\'' +
                ", platform='" + platform + '\'' +
                ", id='" + id + '\'' +
                ", slot=" + slot +
                ", country='" + country + '\'' +
                ", mrank=" + mrank +
                ", autoSquad=" + autoSquad +
                ", waitTime=" + waitTime +
                ", clanId='" + clanId + '\'' +
                ", tier=" + tier +
                ", squad=" + squad +
                ", rank=" + rank +
                ", craftsInfo=" + craftsInfo +
                '}';
    }
    // Getters and setters

    public String getName() {
        return name;
    }

    public int getRank() {
        return rank;
    }

    public String getClanTag() {
        return clanTag;
    }

    public String getPlatform() {
        return platform;
    }

    public List<CraftInfo> getCraftsInfo() {
        return craftsInfo;
    }

    public String getUserId() {
        return id;
    }
}
