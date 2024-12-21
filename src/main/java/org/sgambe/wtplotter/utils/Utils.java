package org.sgambe.wtplotter.utils;

import java.io.*;

public class Utils {
    public static String formatTime(double time) {
        int hours = (int) time / 3600;
        int minutes = (int) time / 60;
        int seconds = (int) time % 60;
        return String.format("%02d:%02d:%02d", hours, minutes, seconds);
    }

    public static void extractCLI() throws IOException {
        //Move file from jar to temp folder
        File tempFolder = new File(System.getProperty("java.io.tmpdir") + "/wtplotter");
        if (!tempFolder.exists()) {
            tempFolder.mkdir();
        }
        File tempFile = new File(tempFolder.getAbsolutePath() + "/wt_ext_cli.exe");
        if (!tempFile.exists()) {
            tempFile.createNewFile();
            try (InputStream in = Utils.class.getResourceAsStream("/wt_ext_cli.exe");
                 OutputStream out = new FileOutputStream(tempFile)) {
                byte[] buffer = new byte[1024];
                int length;
                while ((length = in.read(buffer)) > 0) {
                    out.write(buffer, 0, length);
                }
            }
            System.out.println("Extracted CLI to " + tempFile.getAbsolutePath());
        }
    }
}
