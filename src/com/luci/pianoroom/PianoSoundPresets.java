package com.luci.pianoroom;
/** Sound recipes only: bank, faders, Yamaha, latency and saved scene slots untouched. */
public final class PianoSoundPresets {
 public static final String[] NAMES={"Como antes","Natural","Sala íntima","Concerto","Emocional","Worship"};
 public static final String[] DESCRIPTIONS={"Restaura o som anterior aos testes.","Piano claro, ressonância sutil e pouco ambiente.","Estúdio próximo, quente e discreto.","Sala ampla, cauda equilibrada e notas definidas.","Capela suave, textura leve que cresce entre as frases.","Ambiente longo e shimmer mais presente, com clareza automática."};
 public static ChannelSettings apply(ChannelSettings current,int id){if(id<0||id>=NAMES.length)throw new IllegalArgumentException("Preset");ChannelSettings c=current.copy();c.soundPreset=id;if(id==0){c.hybrid=c.releaseSmooth=c.harmonics=false;c.repetition=0;c.texture=c.ducking=0;return c;}
  c.hybrid=true;c.reverb=100;c.pedal=true;c.contextual=true;c.bankProfile=true;c.harmonics=true;c.releaseSmooth=true;c.repetition=35;c.blend=4;c.chorus=c.delay=0;c.hammer=3;c.keyNoise=2;c.pedalNoise=2;c.bodyEnabled=true;c.bodyAmount=10;c.boardAmount=12;c.bodySize=50;c.boardDamping=55;c.lid=90;
  c.resonance=12;c.spaceRoom=0;c.ambience=25;c.texture=0;c.ducking=25;c.spaceDecay=3;
  if(id==2){c.ambience=45;c.spaceDecay=6;c.bodyAmount=14;c.lid=70;c.ducking=40;}
  if(id==3){c.spaceRoom=1;c.ambience=65;c.spaceDecay=21;c.ducking=55;c.lid=100;}
  if(id==4){c.spaceRoom=2;c.ambience=65;c.spaceDecay=34;c.texture=15;c.ducking=65;c.resonance=10;}
  if(id==5){c.spaceRoom=2;c.ambience=80;c.spaceDecay=35;c.texture=35;c.ducking=80;c.resonance=8;}
  return c;
 }
}
