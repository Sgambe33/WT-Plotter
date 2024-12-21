module org.sgambe.wtplotter {
    requires javafx.fxml;
    requires com.google.gson;
    requires java.prefs;
    requires javafx.controls;

    opens org.sgambe.wtplotter to javafx.fxml;
    opens org.sgambe.wtplotter.replaydata to javafx.base;

    exports org.sgambe.wtplotter;
}