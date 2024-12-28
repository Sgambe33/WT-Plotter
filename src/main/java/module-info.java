module org.sgambe.wtplotter {
    requires javafx.fxml;
    requires com.google.gson;
    requires javafx.controls;
    requires java.prefs;
    requires javafx.swing;
    requires java.net.http;
    requires jdk.compiler;
    requires org.slf4j;

    opens org.sgambe.wtplotter to javafx.fxml;
    opens org.sgambe.wtplotter.replaydata to javafx.base;

    exports org.sgambe.wtplotter;
}