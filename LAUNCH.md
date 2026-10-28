# Plano de lançamento

Roteiro de preparação e conferência do lançamento. O estado efetivo do repositório, das versões e do site deve ser conferido nos endereços oficiais do README. O recebimento por Pix só é ativado após confirmação dos dados pelo mantenedor.

## Após a revisão do mantenedor

1. Confirmar a conta e o nome do repositório. Sugestão: `piano-universe`.
2. Revisar LICENSE e THIRD_PARTY_NOTICES.md. As bibliotecas pessoais do catálogo não são distribuídas.
3. Guardar a chave de assinatura fora do repositório, com backup seguro. O APK de prévia usa a mesma assinatura privada de teste da versão anterior; a identidade e a política de assinatura definitiva devem ser confirmadas antes da primeira versão pública. Não perder a chave usada para atualizações.
4. Configurar o endereço da conta/repositório e o Pix validado em `scripts/configure_site.py`. O payload Pix deve vir do banco ou ser gerado com os dados confirmados do recebedor e CRC correto. A chave aleatória sozinha não confirma beneficiário nem autorização de publicação. Conferir o beneficiário no banco antes de ativar apoio.
5. Criar o repositório público e enviar somente os arquivos do pacote de código. Não enviar os arquivos em docs/downloads nem arquivos de assinatura. O .gitignore já os exclui.
6. Habilitar Discussions, com categorias para dúvidas, ideias e testes. Habilitar relatos privados de segurança. Revisar os formulários de Issues e a moderação da comunidade.
7. Publicar uma Release marcada como pré-lançamento, anexando APK, `Piano-Universe-Bancos-Incluidos.zip`, código-fonte e SHA256SUMS.txt da versão correspondente.
8. Configurar GitHub Pages com GitHub Actions e executar manualmente o workflow `Publicar site`. O download do site aponta para o anexo da Release após configuração.
9. Executar os workflows de verificações e de compilação Android. O workflow Android precisa do anexo de bancos já publicado. Nenhum desses workflows foi executado no GitHub durante a preparação local.
10. Conferir a página pública em computador e celular, o download, a assinatura do APK, Discussions, formulário de Issues e Pix. Não fazer um pagamento real para teste sem autorização do mantenedor.

## Documentação e assinatura
Não são solicitados senha da conta do GitHub, senha bancária nem acesso ao banco. A autorização de publicação e a identificação da conta serão dadas pelo mantenedor após revisar esta prévia.

O fluxo de compilação local já produziu o APK. A compilação automatizada com Gradle no GitHub permanece pendente até existir o repositório e o anexo de bancos.
