# Piano Universe

[Site oficial](https://biblia1924198.github.io/piano-universe/) · [Baixar beta](https://github.com/biblia1924198/piano-universe/releases/tag/v0.27-preview1) · [Comunidade](https://github.com/biblia1924198/piano-universe/discussions)

[English](README.en.md)

Um instrumento Android independente para tocar bibliotecas de piano e outros timbres com seu teclado MIDI.

**Pré-lançamento 0.27-preview1 · Android 8.0 ou superior · ARM64 e x86_64.**

Nome anterior: Piano Room. O identificador `com.luci.pianoroom` permanece para preservar a continuidade das instalações e dos dados.

## O que já existe
Quatro canais de instrumentos, importação de SF2 e de SFZ compatíveis, cenas, efeitos, ajustes de pedal, MIDI, gravação de áudio/vídeo, visualização de piano 3D e benchmarks exportáveis. Motores AudioTrack, AAudio e Oboe, incluindo opção paralela. O suporte a SFZ é parcial: consulte as mensagens do importador.

## Instalação
Baixe o APK na seção Releases. Instale sobre sua versão anterior somente se houver assinatura compatível. Builds de terceiros usam outra assinatura e não atualizam a instalação oficial. Faça backup/exportação das configurações antes de trocar de distribuição.

## Compilar
1. Instale Android Studio com JDK 17, SDK 35, NDK 27.2.12479018 e CMake 3.22.1.
2. Obtenha `Piano-Universe-Bancos-Incluidos.zip` da mesma versão do código.
3. Execute `python scripts/restore_assets.py /caminho/Piano-Universe-Bancos-Incluidos.zip`. O script verifica SHA-256 antes de gravar cada banco.
4. Abra esta pasta no Android Studio, sincronize o projeto e use Build APK(s). AGP 8.8.2 / Gradle 8.10.2. Com Gradle instalado: `gradle assembleDebug`.

Os bancos grandes ficam como anexo da versão, fora do histórico Git. O pacote de fontes público não contém chaves de assinatura. A compilação debug usa a chave de desenvolvimento local do Android. A edição oficial é assinada separadamente pelo mantenedor.

## Participar
Leia [CONTRIBUTING.md](CONTRIBUTING.md). Sugestões e dúvidas vão para Discussions; defeitos reproduzíveis para Issues; mudanças de código chegam por pull requests e passam por revisão do mantenedor.

## Apoiar
O app será gratuito, com apoio voluntário. O site oficial oferece apoio voluntário por chave Pix aleatória informada pelo mantenedor. Confira sempre o beneficiário no aplicativo do banco. Não é preciso doar para usar o aplicativo ou participar.

## Licenças
Código próprio sob MIT, conforme [LICENSE](LICENSE). Dependências e recursos têm suas próprias licenças: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Bibliotecas pessoais de demonstração não fazem parte da distribuição.

## Validação e limites
Esta edição muda a marca e adiciona a apresentação da comunidade; preserva o motor nativo da 0.26. A compilação local e a integridade do APK são verificadas separadamente do uso físico. Testes locais não certificam latência, desempenho ou estabilidade em todos os celulares. Os benchmarks ajudam a relatar o comportamento real.
