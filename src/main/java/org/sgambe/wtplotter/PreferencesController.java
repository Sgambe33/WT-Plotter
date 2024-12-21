package org.sgambe.wtplotter;

import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.Node;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.CheckBox;
import javafx.scene.control.TextField;
import javafx.stage.DirectoryChooser;
import javafx.stage.Stage;

import java.io.File;
import java.util.prefs.Preferences;

public class PreferencesController {
    @FXML
    private TextField replayFolderTextField;

    @FXML
    private Button chooseReplayFolderButton;

    @FXML
    private CheckBox autosaveImagesCheckbox;

    @FXML
    private Button savePrefsButton;

    @FXML
    private Button cancelPrefsButton;

    private Scene firstScene;
    private WTPlotterController wtPlotterController;


    public void setWtPlotterController(WTPlotterController wtPlotterController) {
        this.wtPlotterController = wtPlotterController;
    }
    public void setFirstScene(Scene scene) {
        firstScene = scene;
    }

    public void openFirstScene(ActionEvent actionEvent) {
        Stage primaryStage = (Stage)((Node)actionEvent.getSource()).getScene().getWindow();
        primaryStage.setScene(firstScene);
        wtPlotterController.refreshTree();
    }
    private Preferences prefs = Preferences.userNodeForPackage(PreferencesController.class);

    public void initialize() {
        replayFolderTextField.setText(prefs.get("replayFolder", ""));
        autosaveImagesCheckbox.setSelected(prefs.getBoolean("autosaveImages", false));

        savePrefsButton.setOnAction(event -> {
            prefs.put("replayFolder", replayFolderTextField.getText());
            prefs.putBoolean("autosaveImages", autosaveImagesCheckbox.isSelected());
            openFirstScene(event);
        });

        cancelPrefsButton.setOnAction(this::openFirstScene);

        chooseReplayFolderButton.setOnAction(event -> {
            chooseReplayFolder();
        });
    }

    public void chooseReplayFolder() {
        DirectoryChooser chooser = new DirectoryChooser();
        chooser.setTitle("JavaFX Projects");
        File selectedDirectory = chooser.showDialog(chooseReplayFolderButton.getScene().getWindow());
        if (selectedDirectory != null) {
            replayFolderTextField.setText(selectedDirectory.getAbsolutePath());
            prefs.put("replayFolder", selectedDirectory.getAbsolutePath());
        }
    }
}