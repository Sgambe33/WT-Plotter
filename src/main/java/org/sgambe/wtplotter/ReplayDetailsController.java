package org.sgambe.wtplotter;

import org.sgambe.wtplotter.replaydata.Player;
import org.sgambe.wtplotter.replaydata.PlayerInfo;
import org.sgambe.wtplotter.replaydata.Replay;
import javafx.beans.property.ReadOnlyObjectWrapper;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.MouseButton;
import javafx.scene.text.Text;
import javafx.stage.Stage;

import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.function.Function;

import static org.sgambe.wtplotter.utils.Constants.MAP_NAMES;
import static org.sgambe.wtplotter.utils.Constants.MISSION_MODES;
import static org.sgambe.wtplotter.utils.Utils.formatTime;

public class ReplayDetailsController {
    @FXML
    private Text mapNameLabel;

    @FXML
    private Text startTimeLabel;

    @FXML
    private Label timePlayedLabel;

    @FXML
    private Text resultLabel;

    @FXML
    private Label gameTypeLabel;

    @FXML
    private ImageView mapImage;

    @FXML
    private Tab alliesTab;

    @FXML
    private TableView<Player> alliesTableView;

    @FXML
    private Tab axisTab;

    @FXML
    private TableView<Player> axisTableView;

    private Replay replay;

    /**
     * Sets the file details in the view.
     *
     * @param file The file to display details for.
     */
    public void setFileDetails(File file) throws IOException {
        replay = Replay.fromFile(file);
        String[] test = replay.getBattleType().split("_");
        String gamemode = "_" + test[test.length - 1];

        mapNameLabel.setText("Map: " + MAP_NAMES.get(replay.getLevel()) + " " + MISSION_MODES.get(gamemode));
        startTimeLabel.setText("Start time:" + new Date(replay.getStartTime() * 1000L));
        timePlayedLabel.setText("Time played: " + formatTime(replay.getTimePlayed()));
        resultLabel.setText("Mission result: " + replay.getStatus());
        mapImage.setImage(new Image("http://warthunder-heatmaps.crabdance.com/static/mapsimages/" + replay.getLevel() + "_tankmap_thumb.png"));

        alliesTableView.getItems().addAll(replay.getPlayers().stream().filter(player -> player.getTeam() == 1).toList());
        axisTableView.getItems().addAll(replay.getPlayers().stream().filter(player -> player.getTeam() == 2).toList());
    }

    private void addTableColumn(TableView<Player> tableView, String columnName, String property) {
        TableColumn<Player, ?> column = new TableColumn<>(columnName);
        column.setCellValueFactory(new PropertyValueFactory<>(property));
        tableView.getColumns().add(column);
    }

    private void addCustomIntegerTableColumn(TableView<Player> tableView, String columnName, Function<Player, Integer> valueFunction) {
        TableColumn<Player, Integer> column = new TableColumn<>(columnName);
        column.setCellValueFactory(cellData -> new ReadOnlyObjectWrapper<>(valueFunction.apply(cellData.getValue())));
        tableView.getColumns().add(column);
    }

    private void addCustomStringTableColumn(TableView<Player> tableView, String columnName, Function<Player, String> valueFunction) {
        TableColumn<Player, String> column = new TableColumn<>(columnName);
        column.setCellValueFactory(cellData -> new ReadOnlyObjectWrapper<>(valueFunction.apply(cellData.getValue())));
        tableView.getColumns().add(column);
    }

    private void showPlayerInfoPopup(PlayerInfo player) {
        try {
            FXMLLoader loader = new FXMLLoader(getClass().getResource("PlayerInfoPopupView.fxml"));
            Parent root = loader.load();

            PlayerInfoPopupController controller = loader.getController();
            controller.setPlayerInfo(player);

            Stage stage = new Stage();
            stage.setTitle("Player Info");
            stage.setScene(new Scene(root));
            stage.show();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void initialize() {
        addTableColumn(alliesTableView, "Squad", "squadId");
        addCustomStringTableColumn(alliesTableView, "Player", player -> player.getClanTag() + " " + player.getName());
        addTableColumn(alliesTableView, "Score", "score");
        addTableColumn(alliesTableView, "▭", "kills");
        addTableColumn(alliesTableView, "▮", "groundKills");
        addTableColumn(alliesTableView, "┚", "navalKills");
        addTableColumn(alliesTableView, "▱", "assists");
        addTableColumn(alliesTableView, "△", "captureZone");
        addCustomIntegerTableColumn(alliesTableView, "▯", player -> player.getAiKills() + player.getAiGroundKills() + player.getAiNavalKills());
        addTableColumn(alliesTableView, "⋩", "awardDamage");
        addTableColumn(alliesTableView, "▲", "damageZone");
        addTableColumn(alliesTableView, "▴", "deaths");

        addTableColumn(axisTableView, "Squad", "squadId");
        addCustomStringTableColumn(axisTableView, "Player", player -> player.getClanTag() + " " + player.getName());
        addTableColumn(axisTableView, "Score", "score");
        addTableColumn(axisTableView, "▭", "kills");
        addTableColumn(axisTableView, "▮", "groundKills");
        addTableColumn(axisTableView, "┚", "navalKills");
        addTableColumn(axisTableView, "▱", "assists");
        addTableColumn(axisTableView, "△", "captureZone");
        addCustomIntegerTableColumn(axisTableView, "▯", player -> player.getAiKills() + player.getAiGroundKills() + player.getAiNavalKills());
        addTableColumn(axisTableView, "⋩", "awardDamage");
        addTableColumn(axisTableView, "▲", "damageZone");
        addTableColumn(axisTableView, "▴", "deaths");

        alliesTableView.setRowFactory(tv -> {
            TableRow<Player> row = new TableRow<>();
            row.setOnMouseClicked(event -> {
                if (event.getClickCount() == 1 && !row.isEmpty() && event.getButton() == MouseButton.SECONDARY) {
                    Player rowData = row.getItem();
                    showPlayerInfoPopup(replay.getPlayerInfoByUsername(rowData.getName()));
                }
            });
            return row;
        });

        axisTableView.setRowFactory(tv -> {
            TableRow<Player> row = new TableRow<>();
            row.setOnMouseClicked(event -> {
                if (event.getClickCount() == 1 && !row.isEmpty() && event.getButton() == MouseButton.SECONDARY) {
                    Player rowData = row.getItem();
                    showPlayerInfoPopup(replay.getPlayerInfoByUsername(rowData.getName()));
                }
            });
            return row;
        });
    }
}
