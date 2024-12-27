package org.sgambe.wtplotter.plotter;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import javafx.embed.swing.SwingFXUtils;
import javafx.scene.image.ImageView;
import org.sgambe.wtplotter.replaydata.Replay;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.List;
import java.util.*;
import java.util.stream.Stream;

public class MarkerDrawer {
    private BufferedImage originalMapImage = null;
    private BufferedImage drawedMapImage = null;
    private final List<Position> playerPositionCache;
    private final List<Position> team1PositionCache;
    private final List<Position> team2PositionCache;
    private final List<Position> poi;
    private static boolean havePOIBeenDrawn = false;

    private final ImageView imageView;

    public MarkerDrawer(ImageView imageView) {
        playerPositionCache = new ArrayList<>();
        team1PositionCache = new ArrayList<>();
        team2PositionCache = new ArrayList<>();
        poi = new ArrayList<>();
        this.imageView = imageView;
    }

    public BufferedImage getDrawedMapImage() {
        return drawedMapImage;
    }

    public void setDrawedMapImage(BufferedImage drawedMapImage) {
        this.drawedMapImage = drawedMapImage;
    }

    public BufferedImage getOriginalMapImage() {
        return originalMapImage;
    }

    public void setOriginalMapImage(BufferedImage originalMapImage) {
        this.originalMapImage = originalMapImage;
    }

    public void clearMarkers() {
        playerPositionCache.clear();
        team1PositionCache.clear();
        team2PositionCache.clear();
        poi.clear();
        havePOIBeenDrawn = false;
    }

    public void drawMarkers(BufferedImage displayImage) {
        Graphics2D g = displayImage.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
        g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
        drawMarkers(displayImage, g, playerPositionCache);
        drawMarkers(displayImage, g, team1PositionCache);
        drawMarkers(displayImage, g, team2PositionCache);
        g.dispose();
    }


    private void drawMarkers(BufferedImage displayImage, Graphics2D g, List<Position> positionCache) {
        for (Position pos : positionCache) {
            double x = pos.x();
            double y = pos.y();
            String color = pos.color();
            String type = pos.type();
            Color markerColor = Color.decode(color);
            g.setColor(markerColor);

            if (!Objects.equals(type, "aircraft") && !Objects.equals(type, "airfield") && !Objects.equals(type, "respawn_base_tank") && !Objects.equals(type, "respawn_base_ship") && !Objects.equals(type, "respawn_base_aircraft") && !Objects.equals(type, "capture_zone")) {
                int markerSize = 2;
                int px = (int) (x * displayImage.getWidth());
                int py = (int) (y * displayImage.getHeight());
                g.fillRect(px - markerSize / 2, py - markerSize / 2, markerSize, markerSize);
            }
        }

        imageView.setImage(SwingFXUtils.toFXImage(displayImage, null));
    }

    public void drawSpecialMarkers(BufferedImage displayImage) {
        Graphics2D g = displayImage.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
        g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        g.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
        Map<String, List<Position>> respawnBaseTankGroups = new HashMap<>();

        for (Position pos : poi) {
            if ("capture_zone".equals(pos.type())) {
                drawCaptureZoneMarker(displayImage, g, pos);
            } else if ("respawn_base_tank".equals(pos.type())) {
                respawnBaseTankGroups.computeIfAbsent(pos.color(), k -> new ArrayList<>()).add(pos);
            }
        }

        if (!poi.isEmpty() && !respawnBaseTankGroups.isEmpty()) {
            havePOIBeenDrawn = true;
        }

        for (List<Position> group : respawnBaseTankGroups.values()) {
            if (group.size() >= 5) {
                drawRespawnBaseTank(displayImage, g, group);
            }
        }
    }

    private void drawCaptureZoneMarker(BufferedImage displayImage, Graphics2D g, Position pos) {
        double x = pos.x();
        double y = pos.y();
        int px = (int) (x * displayImage.getWidth());
        int py = (int) (y * displayImage.getHeight());
        g.setColor(Color.YELLOW);
        g.drawRect(px - 10, py - 10, 20, 20);
    }

    private static void drawRespawnBaseTank(BufferedImage displayImage, Graphics2D g, List<Position> group) {
        for (Position pos : group) {
            Color markerColor = Color.MAGENTA;
            g.setColor(markerColor);
            int markerSize = 4;
            int px1 = (int) (pos.x() * displayImage.getWidth());
            int py1 = (int) (pos.y() * displayImage.getHeight());
            g.fillRect(px1 - markerSize / 2, py1 - markerSize / 2, markerSize, markerSize);
        }
    }

    public void addPlayerPosition(Position position) {
        playerPositionCache.add(position);
    }

    public void addTeam1Position(Position position) {
        team1PositionCache.add(position);
    }

    public void addTeam2Position(Position position) {
        team2PositionCache.add(position);
    }

    public void addPOI(Position position) {
        if (!havePOIBeenDrawn) {
            poi.add(position);
        }
    }

    public JsonArray exportPositionsToParquet(Replay replay) {
        List<ParquetPosition> output = new ArrayList<>();
        Stream.of(playerPositionCache, team1PositionCache, team2PositionCache, poi)
                .flatMap(Collection::stream)
                .forEach(position -> output.add(new ParquetPosition(position.x(), position.y(), position.type(), position.icon(), position.timestamp(), replay.getSessionId())));

        JsonArray jsonArray = new JsonArray();
        for (ParquetPosition parquetPosition : output) {
            JsonObject jsonObject = new JsonObject();
            jsonObject.addProperty("x", parquetPosition.x());
            jsonObject.addProperty("y", parquetPosition.y());
            jsonObject.addProperty("type", parquetPosition.type());
            jsonObject.addProperty("icon", parquetPosition.icon());
            jsonObject.addProperty("timestamp", parquetPosition.timestamp());
            jsonObject.addProperty("sessionId", parquetPosition.sessionId());
            jsonArray.add(jsonObject);
        }

        return jsonArray;
    }

    public boolean havePOIBeenDrawn() {
        return havePOIBeenDrawn;
    }

}