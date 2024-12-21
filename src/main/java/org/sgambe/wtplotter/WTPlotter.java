package org.sgambe.wtplotter;

import org.sgambe.wtplotter.utils.Utils;
import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.*;
import java.util.Objects;

public class WTPlotter extends Application {

    public static void main(String[] args) {
        launch(args);
    }

    @Override
    public void start(Stage primaryStage) {
        try {
            FXMLLoader firstPaneLoader = new FXMLLoader(getClass().getResource("MainView.fxml"));
            Parent firstPane = firstPaneLoader.load();
            Scene firstScene = new Scene(firstPane, 960, 600);

            FXMLLoader secondPageLoader = new FXMLLoader(getClass().getResource("PreferencesView.fxml"));
            Parent secondPane = secondPageLoader.load();
            Scene secondScene = new Scene(secondPane, 960, 600);

            WTPlotterController firstController = firstPaneLoader.getController();
            PreferencesController secondController = secondPageLoader.getController();

            firstController.setSecondScene(secondScene);
            secondController.setFirstScene(firstScene);
            secondController.setWtPlotterController(firstController);
            firstScene.getStylesheets().add(Objects.requireNonNull(getClass().getResource("/fontstyle.css")).toExternalForm());

            Utils.extractCLI();

            primaryStage.setTitle("WT Plotter");
            primaryStage.setScene(firstScene);
            primaryStage.show();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}