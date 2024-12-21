package org.sgambe.wtplotter;

import org.sgambe.wtplotter.replaydata.CraftInfo;
import org.sgambe.wtplotter.replaydata.PlayerInfo;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.Tooltip;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.image.ImageView;

public class PlayerInfoPopupController {
    @FXML
    private Label squadronName;

    @FXML
    private Label playerName;

    @FXML
    private Label playerRank;

    @FXML
    private Label rankName;

    @FXML
    private Label playerPlatform;

    @FXML
    private ImageView playerCountryImage;

    @FXML
    private TableView<CraftInfo> playerVehiclesTableView;

    private PlayerInfo playerInfo;

    public void setPlayerInfo(PlayerInfo player) {
        this.playerInfo = player;
        squadronName.setText(player.getClanTag());
        squadronName.setStyle("-fx-font-family: \"symbols_skyquake\";");
        playerName.setText(player.getName());
        playerName.setTooltip(new Tooltip(player.getUserId()));
        playerRank.setText("Level " + player.getRank());
        rankName.setText("Placeholder");
        playerPlatform.setText(player.getPlatform());
        //ADD images

        playerVehiclesTableView.getItems().addAll(player.getCraftsInfo());
    }

    public void initialize() {
        TableColumn<CraftInfo, String> column = new TableColumn<>("Vehicle");
        column.setCellValueFactory(new PropertyValueFactory<>("name"));
        playerVehiclesTableView.getColumns().add(column);

        TableColumn<CraftInfo, Boolean> column2 = new TableColumn<>("Unused");
        column2.setCellValueFactory(new PropertyValueFactory<>("rankUnused"));
        playerVehiclesTableView.getColumns().add(column2);

        TableColumn<CraftInfo, String> column3 = new TableColumn<>("Type");
        column3.setCellValueFactory(new PropertyValueFactory<>("type"));
        playerVehiclesTableView.getColumns().add(column3);
    }
}
