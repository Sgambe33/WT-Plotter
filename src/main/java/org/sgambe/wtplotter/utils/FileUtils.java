package org.sgambe.wtplotter.utils;

import javax.imageio.IIOImage;
import javax.imageio.ImageIO;
import javax.imageio.ImageWriteParam;
import javax.imageio.ImageWriter;
import javax.imageio.stream.ImageOutputStream;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

public class FileUtils {

    public static void saveFilteredPlotToDisk(BufferedImage mapImage, String currEpoch) {
        if (mapImage == null) {
            return;
        }

        try {
            File outputFile = new File(System.getProperty("user.home") + "/plots/" + currEpoch + "_wtplot.png");
            if (!outputFile.getParentFile().exists()) {
                if (!outputFile.getParentFile().mkdirs()) {
                    System.err.println("Failed to create directories for: " + outputFile.getParentFile().getAbsolutePath());
                    return;
                }
            }
            ImageWriter writer = ImageIO.getImageWritersByFormatName("png").next();
            try (ImageOutputStream ios = ImageIO.createImageOutputStream(outputFile)) {
                writer.setOutput(ios);
                ImageWriteParam param = writer.getDefaultWriteParam();
                if (param.canWriteCompressed()) {
                    param.setCompressionMode(ImageWriteParam.MODE_EXPLICIT);
                    param.setCompressionType("Deflate");
                    param.setCompressionQuality(0.8f);
                }
                writer.write(null, new IIOImage(mapImage, null, null), param);
            } finally {
                writer.dispose();
            }
        } catch (IOException e) {
        }
    }
}