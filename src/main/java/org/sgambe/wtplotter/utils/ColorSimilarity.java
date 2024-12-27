package org.sgambe.wtplotter.utils;

public class ColorSimilarity {

    public static int[] hexToRgb(String hex) {
        hex = hex.replace("#", "");
        int r = Integer.parseInt(hex.substring(0, 2), 16);
        int g = Integer.parseInt(hex.substring(2, 4), 16);
        int b = Integer.parseInt(hex.substring(4, 6), 16);

        return new int[] {r, g, b};
    }

    public static String getDominantColor(String hex) {
        int[] rgb = hexToRgb(hex);
        int r = rgb[0];
        int g = rgb[1];
        int b = rgb[2];

        if (r >= g && r >= b) {
            return "Red";
        } else if (g >= r && g >= b) {
            return "Green";
        } else {
            return "Blue";
        }
    }
}
