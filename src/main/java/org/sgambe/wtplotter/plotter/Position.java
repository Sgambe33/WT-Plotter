package org.sgambe.wtplotter.plotter;

import java.util.Date;

public record Position(double x, double y, String color, String type, String icon, long timestamp) {
    public Position {
        if (x < 0 || x > 1) {
            throw new IllegalArgumentException("x must be between 0 and 1");
        }
        if (y < 0 || y > 1) {
            throw new IllegalArgumentException("y must be between 0 and 1");
        }
    }

    public boolean isCaptureZone() {
        return type.equalsIgnoreCase("capture_zone");
    }

    public boolean isRespawnBaseTank() {
        return type.equalsIgnoreCase("respawn_base_tank");
    }

    public boolean isPlayer() {
        return "Player".equals(icon);
    }

    public boolean isAirfield() {
        return type.equalsIgnoreCase("airfield");
    }

    public boolean isAircraft() {
        return type.equalsIgnoreCase("aircraft");
    }

    public boolean isRespawnBaseFighter() {
        return type.equalsIgnoreCase("respawn_base_fighter");
    }
}
