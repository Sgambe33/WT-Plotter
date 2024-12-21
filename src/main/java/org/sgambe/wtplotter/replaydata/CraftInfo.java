package org.sgambe.wtplotter.replaydata;

import com.google.gson.JsonObject;

public class CraftInfo {
    private String name;
    private String type;
    private boolean rankUnused;
    private int mrank;
    private int rank;


    public CraftInfo fromJson(JsonObject craftInfoObject) {
        this.name = craftInfoObject.get("name").getAsString();
        this.type = craftInfoObject.get("type").getAsString();
        this.rankUnused = craftInfoObject.get("rankUnused").getAsBoolean();
        this.mrank = craftInfoObject.get("mrank").getAsInt();
        this.rank = craftInfoObject.get("rank").getAsInt();
        return this;

    }

    public int getRank() {
        return rank;
    }

    public int getMrank() {
        return mrank;
    }

    public boolean isRankUnused() {
        return rankUnused;
    }

    public String getType() {
        return type;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return "CraftInfo{" + "name='" + name + '\'' + ", type='" + type + '\'' + ", rankUnused=" + rankUnused + ", mrank=" + mrank + ", rank=" + rank + '}';
    }
}
