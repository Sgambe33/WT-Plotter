package org.sgambe.wtplotter.plotter;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import org.sgambe.wtplotter.utils.ColorSimilarity;


import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Date;

public class HttpFetcher {
    private static final String DATA_URL = "http://localhost:8111/map_obj.json";
    private static final String MAP_URL = "http://localhost:8111/map.img";
    private static final String MAP_INFO = "http://localhost:8111/map_info.json";

    private final MarkerDrawer markerDrawer;
    private long matchStartTime = 0;

    public HttpFetcher(MarkerDrawer markerDrawer) {
        this.markerDrawer = markerDrawer;
    }

    public void setMatchStartTime(long matchStartTime) {
        this.matchStartTime = matchStartTime;
    }

    private JsonElement fetchJsonElement(String url) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setRequestMethod("GET");

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {
            StringBuilder json = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                json.append(line);
            }
            return JsonParser.parseString(json.toString());
        }
    }

    public boolean isMatchRunning() {
        try {
            JsonElement mapInfo = fetchJsonElement(MAP_INFO);
            if (mapInfo.isJsonObject()) {
                JsonObject mapInfoObject = mapInfo.getAsJsonObject();
                return mapInfoObject.has("valid") && mapInfoObject.get("valid").getAsBoolean();
            }
            return false;
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * Fetches the map image from the server and displays it in the GUI.
     *
     * @throws IOException If an error occurs while fetching the map.
     */
    public void fetchAndDisplayMap() throws IOException {
        if (!isMatchRunning()) return;
        BufferedImage mapImage = fetchMapImage();
        if (mapImage != null && mapImage.getWidth() == 2048) {
            markerDrawer.setOriginalMapImage(mapImage);
        }else{
            System.err.println("Invalid map image: " + (mapImage == null ? "null" : mapImage.getWidth()));
        }
    }

    /**
     * Fetches the map image from the server.
     *
     * @return The map image, or null if an error occurred.
     */
    private static BufferedImage fetchMapImage() throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(MAP_URL).openConnection();
        connection.setRequestMethod("GET");

        try (InputStream inputStream = connection.getInputStream()) {
            return ImageIO.read(inputStream);
        }
    }


    /**
     * Fetches the object data from the server and adds it to the marker drawer.
     */
    public void fetchMapObjects() {
        try {
            JsonElement objects = fetchJsonElement(DATA_URL);
            if (!objects.isJsonArray()) return;
            JsonArray objectArray = objects.getAsJsonArray();
            for (JsonElement element : objectArray) {
                if (element.getAsJsonObject().has("x") && element.getAsJsonObject().has("y")) {
                    Position position = getPositionFromJsonElement(element);
                    if (position == null) continue;

                    if (position.isCaptureZone() || position.isRespawnBaseTank()) {
                        if (!markerDrawer.havePOIBeenDrawn()) {
                            markerDrawer.addPOI(position);
                        }
                    } else if (position.isPlayer()) {
                        markerDrawer.addPlayerPosition(position);
                    } else if (ColorSimilarity.getDominantColor(position.color()).equalsIgnoreCase("blue")) {
                        markerDrawer.addTeam1Position(position);
                    } else if (ColorSimilarity.getDominantColor(position.color()).equalsIgnoreCase("red")) {
                        markerDrawer.addTeam2Position(position);
                    } else if (ColorSimilarity.getDominantColor(position.color()).equalsIgnoreCase("green")) {
                        markerDrawer.addTeam1Position(position);
                    } else {
                        System.err.println("Unknown color: " + position.color());
                    }
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

    }

    private Position getPositionFromJsonElement(JsonElement element) {
        JsonObject obj = element.getAsJsonObject();

        double x = obj.get("x").getAsDouble();
        double y = obj.get("y").getAsDouble();
        String color = obj.has("color") ? obj.get("color").getAsString() : "#FFFFFF";
        String type = obj.has("type") ? obj.get("type").getAsString() : "unknown";
        String icon = obj.has("icon") ? obj.get("icon").getAsString() : "unknown";

        Date now = new Date();
        long timeSinceBeginning = now.getTime() - matchStartTime;
        if (type.equalsIgnoreCase("aircraft") || type.equalsIgnoreCase("airfield") || type.equalsIgnoreCase("respawn_base_fighter") || x < 0 || y < 0 || x > 1 || y > 1) return null;
        if (type.equalsIgnoreCase("respawn_base_tank")) return new Position(x, y, "#ff00ff", type, icon, 0);
        if (type.equalsIgnoreCase("capture_zone")) return new Position(x, y, "#ffff00", type, icon, 0);
        if (icon.equalsIgnoreCase("Player")) return new Position(x, y, "#00ffff", type, icon, timeSinceBeginning);
        return new Position(x, y, color, type, icon, timeSinceBeginning);
    }

    public long getMatchStartTime() {
        return matchStartTime;
    }

    public boolean isPlayerOnTank() {
        try {
            JsonElement response = fetchJsonElement("http://localhost:8111/indicators");
            if (response.isJsonObject()) {
                JsonObject responseObj = response.getAsJsonObject();
                boolean result = responseObj.has("valid") && responseObj.get("valid").getAsBoolean();
                result = result && (responseObj.has("army") && responseObj.get("army").getAsString().equalsIgnoreCase("tank"));
                return result;
            }
            return false;
        } catch (IOException e) {
            System.err.println("IO exception while fetching indicators: " + e.getMessage());
            return false;
        }
    }
}