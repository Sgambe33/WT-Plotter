package org.sgambe.wtplotter;

import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.AnchorPane;
import javafx.stage.Stage;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.prefs.Preferences;

public class WTPlotterController {
    @FXML
    private MenuItem preferencesMenuItem;

    @FXML
    private Button plotterButton;

    @FXML
    private Button replaysButton;

    @FXML
    private TreeView<String> replayListTree;

    @FXML
    private AnchorPane rightPane;

    @FXML
    private Label bottomStatusLabel;

    private ReplayDetailsController replayDetailsController;
    private PlotterController plotterController;

    private Map<TreeItem<String>, File> treeItemToFileMap = new HashMap<>();

    private Scene secondScene;

    public void setSecondScene(Scene scene) {
        secondScene = scene;
    }

    public void openSecondScene(ActionEvent actionEvent) {
        Stage primaryStage = (Stage) preferencesMenuItem.getParentPopup().getOwnerWindow();
        primaryStage.setScene(secondScene);
    }

    public void refreshTree() {
        populateTree();
    }

    private void populateTree() {
        Preferences prefs = Preferences.userNodeForPackage(PreferencesController.class);
        if (prefs.get("replayFolder", "").isEmpty()) {
            System.err.println("Replay folder not set in preferences");
            return;
        }
        File replayFolder = new File(prefs.get("replayFolder", ""));
        File[] replayFiles = replayFolder.listFiles((dir, name) -> name.endsWith(".wrpl"));

        if (replayFiles == null) return; // Exit if folder is empty or doesn't exist

        Map<String, List<File>> replayMap = new HashMap<>();

        // Group replay files by date (yyyy.mm.dd)
        for (File replayFile : replayFiles) {
            String date = replayFile.getName().substring(1, 11);
            replayMap.computeIfAbsent(date, k -> new ArrayList<>()).add(replayFile);
        }

        // Root node for the TreeView
        TreeItem<String> root = new TreeItem<>("Replays");
        root.setExpanded(true);

        for (Map.Entry<String, List<File>> entry : replayMap.entrySet()) {
            TreeItem<String> dateItem = new TreeItem<>(entry.getKey());
            for (File file : entry.getValue()) {
                TreeItem<String> fileItem = new TreeItem<>(file.getName());
                treeItemToFileMap.put(fileItem, file); // Map the TreeItem to its File
                dateItem.getChildren().add(fileItem);
            }
            root.getChildren().add(dateItem);
        }

        replayListTree.setRoot(root);
    }

    private void handleTreeViewClick(MouseEvent event) {
        TreeItem<String> selectedItem = replayListTree.getSelectionModel().getSelectedItem();

        if (selectedItem != null && treeItemToFileMap.containsKey(selectedItem)) {
            File selectedFile = treeItemToFileMap.get(selectedItem);
            loadDetailsView(selectedFile);
        }
    }

    private void loadDetailsView(File file) {
        try {
            FXMLLoader loader = new FXMLLoader(getClass().getResource("ReplayDetailsView.fxml"));
            Parent detailsView = loader.load();

            ReplayDetailsController detailsController = loader.getController();
            detailsController.setFileDetails(file);

            replayDetailsController = loader.getController();


            rightPane.getChildren().clear();
            rightPane.getChildren().add(detailsView);

            AnchorPane.setTopAnchor(detailsView, 0.0);
            AnchorPane.setBottomAnchor(detailsView, 0.0);
            AnchorPane.setLeftAnchor(detailsView, 0.0);
            AnchorPane.setRightAnchor(detailsView, 0.0);

        } catch (IOException e) {
            e.printStackTrace();
            System.err.println("Failed to load ReplayDetailsView.fxml");
        }
    }

    public void loadPlotterView(){
        try {
            FXMLLoader loader = new FXMLLoader(getClass().getResource("PlotterView.fxml"));
            Parent plotterView = loader.load();

            plotterController = loader.getController();
            plotterController.setBottomStatusLabel(bottomStatusLabel);

            rightPane.getChildren().clear();
            rightPane.getChildren().add(plotterView);

            AnchorPane.setTopAnchor(plotterView, 0.0);
            AnchorPane.setBottomAnchor(plotterView, 0.0);
            AnchorPane.setLeftAnchor(plotterView, 0.0);
            AnchorPane.setRightAnchor(plotterView, 0.0);

        } catch (IOException e) {
            e.printStackTrace();
            System.err.println("Failed to load ReplayDetailsView.fxml");
        }
    }

    public void initialize() {
        populateTree();
        preferencesMenuItem.setOnAction(this::openSecondScene);
        replayListTree.setOnMouseClicked(this::handleTreeViewClick);
        replayListTree.setDisable(true);
        loadPlotterView();

        replaysButton.setOnAction(event -> {
            plotterButton.setDisable(false);
            replaysButton.setDisable(true);
            replayListTree.setDisable(false);
            plotterController.shutdown();

            try {
                FXMLLoader loader = new FXMLLoader(getClass().getResource("ReplayDetailsView.fxml"));
                Parent replayView = loader.load();
                //WILL LOAD LATEST REPLAY WHEN SQLITE IS IMPLEMENTED
                rightPane.getChildren().clear();
                rightPane.getChildren().add(replayView);



                AnchorPane.setTopAnchor(replayView, 0.0);
                AnchorPane.setBottomAnchor(replayView, 0.0);
                AnchorPane.setLeftAnchor(replayView, 0.0);
                AnchorPane.setRightAnchor(replayView, 0.0);

            } catch (IOException e) {
                e.printStackTrace();
                System.err.println("Failed to load ReplayView.fxml");
            }
        });

        plotterButton.setOnAction(event -> {
            plotterButton.setDisable(true);
            replaysButton.setDisable(false);
            replayListTree.setDisable(true);
            loadPlotterView();
        });
    }

}
