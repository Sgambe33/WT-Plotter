package org.sgambe.wtplotter.plotter;

public record ParquetPosition(double x, double y, String type, String icon, long timestamp, String sessionId) {
    public ParquetPosition {
        if (x < 0 || x > 1) {
            throw new IllegalArgumentException("x must be between 0 and 1");
        }
        if (y < 0 || y > 1) {
            throw new IllegalArgumentException("y must be between 0 and 1");
        }
    }
}
