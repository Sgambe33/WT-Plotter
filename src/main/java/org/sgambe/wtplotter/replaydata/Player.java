package org.sgambe.wtplotter.replaydata;

import com.google.gson.JsonObject;

public class Player {
    private String name;
    private String clanTag;
    private int kills;
    private int groundKills;
    private int navalKills;
    private int teamKills;
    private int aiKills;
    private int aiGroundKills;
    private int aiNavalKills;
    private int assists;
    private int deaths;
    private int captureZone;
    private int damageZone;
    private int score;
    private int awardDamage;
    private int missileEvades;
    private String userId;
    private int squadId;
    private int autoSquad;
    private int team;

    public Player fromJson(JsonObject json){
        this.name = json.get("name").getAsString();
        this.clanTag = json.get("clanTag").getAsString();
        this.kills = json.get("kills").getAsInt();
        this.groundKills = json.get("groundKills").getAsInt();
        this.navalKills = json.get("navalKills").getAsInt();
        this.teamKills = json.get("teamKills").getAsInt();
        this.aiKills = json.get("aiKills").getAsInt();
        this.aiGroundKills = json.get("aiGroundKills").getAsInt();
        this.aiNavalKills = json.get("aiNavalKills").getAsInt();
        this.assists = json.get("assists").getAsInt();
        this.deaths = json.get("deaths").getAsInt();
        this.captureZone = json.get("captureZone").getAsInt();
        this.damageZone = json.get("damageZone").getAsInt();
        this.score = json.get("score").getAsInt();
        this.awardDamage = json.get("awardDamage").getAsInt();
        this.missileEvades = json.get("missileEvades").getAsInt();
        this.userId = json.get("userId").getAsString();
        this.squadId = json.get("squadId").getAsInt(); //- (this.team == 1 ? 1000 : 2000);
        this.autoSquad = json.get("autoSquad").getAsInt();
        this.team = json.get("team").getAsInt();
        return this;
    }

    @Override
    public String toString(){
        return "Player{" +
                "name='" + name + '\'' +
                ", clanTag='" + clanTag + '\'' +
                ", kills=" + kills +
                ", groundKills=" + groundKills +
                ", navalKills=" + navalKills +
                ", teamKills=" + teamKills +
                ", aiKills=" + aiKills +
                ", aiGroundKills=" + aiGroundKills +
                ", aiNavalKills=" + aiNavalKills +
                ", assists=" + assists +
                ", deaths=" + deaths +
                ", captureZone=" + captureZone +
                ", damageZone=" + damageZone +
                ", score=" + score +
                ", awardDamage=" + awardDamage +
                ", missileEvades=" + missileEvades +
                ", userId='" + userId + '\'' +
                ", squadId=" + squadId +
                ", autoSquad=" + autoSquad +
                ", team=" + team +
                '}';
    }

    public String getName() {
        return name;
    }

    public String getClanTag() {
        return clanTag;
    }

    public int getKills() {
        return kills;
    }

    public int getGroundKills() {
        return groundKills;
    }

    public int getNavalKills() {
        return navalKills;
    }

    public int getTeamKills() {
        return teamKills;
    }

    public int getAiKills() {
        return aiKills;
    }

    public int getAiGroundKills() {
        return aiGroundKills;
    }

    public int getAiNavalKills() {
        return aiNavalKills;
    }

    public int getAssists() {
        return assists;
    }

    public int getDeaths() {
        return deaths;
    }

    public int getCaptureZone() {
        return captureZone;
    }

    public int getDamageZone() {
        return damageZone;
    }

    public int getScore() {
        return score;
    }

    public int getAwardDamage() {
        return awardDamage;
    }

    public int getMissileEvades() {
        return missileEvades;
    }

    public String getUserId() {
        return userId;
    }

    public int getSquadId() {
        return squadId;
    }

    public int getAutoSquad() {
        return autoSquad;
    }

    public int getTeam() {
        return team;
    }

    // Getters and setters
}
