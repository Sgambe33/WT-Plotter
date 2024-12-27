package org.sgambe.wtplotter;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import javafx.application.Platform;
import javafx.concurrent.Task;
import javafx.fxml.FXML;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.image.ImageView;
import javafx.scene.layout.StackPane;
import org.sgambe.wtplotter.utils.FileUtils;
import org.sgambe.wtplotter.plotter.HttpFetcher;
import org.sgambe.wtplotter.plotter.MarkerDrawer;
import org.sgambe.wtplotter.replaydata.Replay;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;
import java.util.Comparator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.prefs.Preferences;
import java.util.stream.Collectors;

public class PlotterController {
    @FXML
    private StackPane mainStackPane;
    private boolean running = true;
    private static Label bottomStatusLabel;
    private ExecutorService executorService;


    public void setBottomStatusLabel(Label bottomStatusLabel) {
        PlotterController.bottomStatusLabel = bottomStatusLabel;
    }

    private static boolean shouldLoadMap(MarkerDrawer markerDrawer, HttpFetcher httpFetcher) {
        return markerDrawer.getOriginalMapImage() == null && httpFetcher.isMatchRunning();
    }

    private static void loadMap(HttpFetcher httpFetcher) {
        try {
            httpFetcher.fetchAndDisplayMap();
            System.out.println("Map loaded.");
        } catch (IOException e) {
            System.err.println("Error loading map: " + e.getMessage());
        }
    }

    private static boolean shouldUpdateMarkers(MarkerDrawer markerDrawer, HttpFetcher httpFetcher) {
        return markerDrawer.getOriginalMapImage() != null && httpFetcher.isMatchRunning();
    }

    private static void updateMarkers(MarkerDrawer markerDrawer, HttpFetcher httpFetcher) {
        if (httpFetcher.isPlayerOnTank()) {
            httpFetcher.fetchMapObjects();
            markerDrawer.setDrawedMapImage(markerDrawer.getOriginalMapImage());
            markerDrawer.drawSpecialMarkers(markerDrawer.getDrawedMapImage());
            markerDrawer.drawMarkers(markerDrawer.getDrawedMapImage());
        }
    }

    private static boolean shouldEndMatch(MarkerDrawer markerDrawer, HttpFetcher httpFetcher) {
        return markerDrawer.getOriginalMapImage() != null && !httpFetcher.isMatchRunning();
    }

    private static void uploadMatch(MarkerDrawer markerDrawer, Replay header, String uploader) throws IOException {
        JsonObject headerMap = new JsonObject();
        headerMap.addProperty("sessionId", header.getSessionId());
        headerMap.addProperty("uploader", uploader);
        headerMap.addProperty("startTime", header.getStartTime());
        headerMap.addProperty("map", header.getLevel());
        headerMap.addProperty("gameMode", header.getBattleType());

        JsonArray positions = markerDrawer.exportPositionsToParquet(header);

        JsonObject data = new JsonObject();
        data.add("replayHeader", headerMap);
        data.add("positions", positions);

        HttpURLConnection connection = (HttpURLConnection) new URL("http://89.168.30.170/uploadPositions").openConnection();
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Content-Type", "application/json");
        connection.setDoOutput(true);
        try (OutputStream os = connection.getOutputStream()) {
            os.write(data.toString().getBytes());
            os.flush();
        }

        int responseCode = connection.getResponseCode();
        try {
            String responseBody = new BufferedReader(new InputStreamReader(connection.getInputStream())).lines().collect(Collectors.joining("\n"));
            if (responseCode == 200) {
                System.out.println("Match uploaded successfully.");
            } else {
                System.err.println("Failed to upload match. Response code: " + responseCode + ", response body: " + responseBody);
            }
        }catch (IOException e){
            System.err.println("Error reading response body: " + e.getMessage());
        }
    }

    private static File getLatestReplay(File replayDirectory) {
        File[] files = replayDirectory.listFiles();
        if (files == null || files.length == 0) {
            return null;
        }

        long sixtySecondsAgo = System.currentTimeMillis() - 120000;
        files = Arrays.stream(files).filter(file -> file.getPath().endsWith(".wrpl") && file.lastModified() >= sixtySecondsAgo).toArray(File[]::new);

        if (files.length == 0) {
            return null;
        }

        Arrays.sort(files, Comparator.comparingLong(File::lastModified).reversed());
        return files[0];
    }


    private static void endMatch(MarkerDrawer markerDrawer) {
        String currEpoch = String.valueOf(System.currentTimeMillis());
        try {
            FileUtils.saveFilteredPlotToDisk(markerDrawer.getDrawedMapImage(), currEpoch);
            Preferences prefs = Preferences.userNodeForPackage(PreferencesController.class);
            File latestReplay;
            int retries = 60;
            do {
                latestReplay = getLatestReplay(new File(prefs.get("replayFolder", "")));
                if (latestReplay == null) {
                    Thread.sleep(1000);
                }
            } while (latestReplay == null && retries-- > 0);
            if (latestReplay != null) {
                System.out.println("Latest replay file: " + latestReplay);
                Replay replay = Replay.fromFile(latestReplay);
                String uploader = replay.getAuthorUserId();
                if (uploader != null) {
                    changeStatus("Uploading match...");
                    uploadMatch(markerDrawer, replay, uploader);
                } else {
                    changeStatus("Failed to get user UID");
                    System.out.println("Failed to get user UID. Data will not be validated against replay.");
                }
            } else {
                changeStatus("Failed to find latest replay file");
                System.err.println("Failed to find latest replay file.");
            }
        } catch (FileNotFoundException e) {
            System.err.println("Error exporting position cache or saving plot: " + e.getMessage());
            throw new RuntimeException(e);
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }

        markerDrawer.clearMarkers();
        markerDrawer.setOriginalMapImage(null);
        markerDrawer.setDrawedMapImage(null);
    }

    private static void changeStatus(String s) {
        Platform.runLater(() -> bottomStatusLabel.setText(s));
    }

    public void initialize() {
        ImageView imageView = new ImageView("https://vistapointe.net/images/unknown-2.jpg");
        imageView.setPreserveRatio(true);
        imageView.fitWidthProperty().bind(mainStackPane.widthProperty());
        imageView.fitHeightProperty().bind(mainStackPane.heightProperty());
        mainStackPane.getChildren().add(imageView);
        StackPane.setAlignment(imageView, Pos.CENTER);

        MarkerDrawer markerDrawer = new MarkerDrawer(imageView);
        HttpFetcher httpFetcher = new HttpFetcher(markerDrawer);

        Task<Void> fetchPositionsTask = new Task<>() {
            @Override
            protected Void call() throws Exception {
                while (running) {
                    changeStatus("Awaiting match start...");
                    if (shouldLoadMap(markerDrawer, httpFetcher)) {
                        loadMap(httpFetcher);
                        changeStatus("Match started: loading map...");
                    } else if (shouldUpdateMarkers(markerDrawer, httpFetcher)) {
                        if (httpFetcher.getMatchStartTime() == 0) {
                            httpFetcher.setMatchStartTime(System.currentTimeMillis());
                            changeStatus("Match started: fetching positions...");
                        }
                        updateMarkers(markerDrawer, httpFetcher);
                    } else if (shouldEndMatch(markerDrawer, httpFetcher)) {
                        changeStatus("Match ended...");
                        httpFetcher.setMatchStartTime(0);
                        endMatch(markerDrawer);
                        markerDrawer.setOriginalMapImage(null);
                        markerDrawer.setDrawedMapImage(null);
                    }
                    Thread.sleep(1000);
                }
                System.out.println("Shutting down thread " + Thread.currentThread().getName());
                return null;
            }
        };

        executorService = Executors.newSingleThreadExecutor();
        executorService.submit(fetchPositionsTask);
    }

    public void shutdown() {
        running = false;
        executorService.shutdown();
        try {
            if (!executorService.awaitTermination(5, TimeUnit.SECONDS)) {
                executorService.shutdownNow();
            }
        } catch (InterruptedException e) {
            executorService.shutdownNow();
        }
    }


}
