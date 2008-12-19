package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RecordInterface;
import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.MemoryImageSource;
import java.awt.Color;

public class RawDecoder {

  public RawDecoder(Graphics g, RfbInputStream is) {
    setGraphics(g);
    setRfbInputStream(is);
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);
  }

  public RawDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                    int frameBufferH) {
    setGraphics(g);
    setRfbInputStream(is);
    setFrameBufferSize(frameBufferW, frameBufferH);
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);
  }

  public void setRfbInputStream(RfbInputStream is) {
    rfbis = is;
  }

  public void setGraphics(Graphics g) {
    graphics = g;
  }

  public void setBPP(int bpp) {
    bytesPerPixel = bpp;
  }

  public int getBPP() {
    return bytesPerPixel;
  }

  public void setFrameBufferSize(int w, int h) {
    framebufferWidth = w;
    framebufferHeight = h;
  }

  protected int bytesPerPixel = 4;
  protected int framebufferWidth = 0;
  protected int framebufferHeight = 0;
  protected RfbInputStream rfbis = null;
  protected Graphics graphics = null;
  protected RecordInterface rec = null;

  protected static byte []pixels8 = null;
  protected static int []pixels24 = null;
  protected static MemoryImageSource pixelsSource = null;
  protected static Image rawPixelsImage = null;

  private static ColorModel cm8 = null;
  private static ColorModel cm24 = null;
  private static Color []color256 = null;
}