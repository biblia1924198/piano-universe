package com.luci.pianoroom;
import java.io.*;import java.util.Base64;
/** The Yamaha is a live stereo source, never a downloaded sample bank. */
public final class YamahaAudioSettings {
 private YamahaAudioSettings(){}
 public static String encode(ChannelSettings c)throws IOException{ByteArrayOutputStream bytes=new ByteArrayOutputStream();try(DataOutputStream o=new DataOutputStream(bytes)){o.writeInt(0x50525931);c.write(o);c.writePiano(o);c.writeBody(o);c.writeSpace(o);}return Base64.getEncoder().encodeToString(bytes.toByteArray());}
 public static ChannelSettings decode(String text)throws IOException{try(DataInputStream i=new DataInputStream(new ByteArrayInputStream(Base64.getDecoder().decode(text)))){if(i.readInt()!=0x50525931)throw new IOException("Ajustes Yamaha inválidos");ChannelSettings c=ChannelSettings.read(i);c.readPiano(i);c.readBody(i);c.readSpace(i);if(i.available()!=0)throw new IOException("Ajustes Yamaha extras");return c;}catch(IllegalArgumentException e){throw new IOException(e);}}
 public static String routeProblem(boolean inputUsb,boolean outputUsb,boolean sameDevice,boolean wetOnly,boolean loopbackOff){if(!inputUsb)return "Escolha uma entrada de áudio USB do Yamaha; o microfone do celular não é usado.";if(wetOnly){if(!outputUsb||!sameDevice)return "Para efeitos adicionais nas caixas, selecione a saída USB do mesmo Yamaha.";if(!loopbackOff)return "Desligue Audio Loop Back no Yamaha e confirme abaixo antes de ativar.";}else if(sameDevice)return "Som completo exige outra saída: fones/interface ligados ao celular. Nas caixas do Yamaha use Efeitos adicionais.";return "";}
}
