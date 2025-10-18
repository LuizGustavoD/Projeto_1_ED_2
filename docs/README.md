# Banco de Dados de Imagens PGM - Projeto ED2

## 📋 Descrição
Sistema de banco de dados para armazenamento e manipulação de imagens PGM com compressão RLE (Run-Length Encoding).

## 🏗️ Estrutura do Projeto
```
Projeto 1 ED 2/
├── main.c              # Arquivo principal
├── main.exe            # Executável
├── app/                # Módulos da aplicação
│   ├── app.c/.h        # Interface principal
│   ├── pgm.c/.h        # Manipulação PGM
│   ├── bin.c/.h        # Operações binárias
│   ├── keys.c/.h       # Sistema de índices
│   ├── pgm_operations.c/.h # Operações de imagem (RLE, limiarização)
│   └── manage_data.c/.h    # Gerenciamento do banco
├── models/
│   └── structure.h     # Estruturas de dados
├── data/               # Banco de dados
│   ├── imagens.bin     # Arquivo de dados RLE
│   └── indices.bin     # Arquivo de índices
├── docs/               # Documentação
└── images/             # Imagens de exemplo
```

## 🚀 Como Compilar
```bash
gcc main.c app/*.c -o main
```

## 🎮 Como Usar

### Executar o programa:
```bash
./main.exe
```

### Menu Principal:
1. **Carregar Imagem do Banco de Dados**
   - Busca imagem por nome no banco
   - Permite aplicar operações (negativação, limiarização)
   - Salva resultados no banco

2. **Salvar Imagem no Banco de Dados**
   - Carrega imagem PGM do disco
   - Aplica compressão RLE
   - Adiciona ao índice

3. **Deletar Imagem (flag de remoção)**
   - Marca imagem para deleção (soft delete)
   - Não remove fisicamente até compactação

4. **Comprimir Arquivo (remover deletados)**
   - Remove definitivamente imagens marcadas
   - Compacta o banco de dados

5. **Listar Imagens no Banco**
   - Mostra todas as imagens indexadas

6. **Informações do Programa**
   - Detalhes sobre o projeto

7. **Sair**

## 🔧 Funcionalidades Técnicas

### Compressão RLE
- Formato otimizado para imagens binárias (limiarizadas)
- Estrutura: `[flag_removed, width, height, grey_levels, primeiro_pixel, count1, count2, ...]`

### Sistema de Índices
- Busca rápida por nome
- Estrutura: `{nome[100], offset}`

### Operações de Imagem
- **Limiarização**: Converte para binário (0 ou grey_levels)
- **Negativação**: Inverte cores

### Gerenciamento de Dados
- **Soft Delete**: Marca para deleção sem remover
- **Compactação**: Remove dados deletados definitivamente

## 📝 Exemplo de Uso

1. Salvar uma imagem:
   - Opção 2 → Digite caminho da imagem PGM
   - Sistema aplica RLE e indexa

2. Deletar uma imagem:
   - Opção 3 → Digite nome da imagem
   - Confirme a deleção (marca flag)

3. Compactar banco:
   - Opção 4 → Remove imagens deletadas permanentemente

## 👨‍🎓 Informações Acadêmicas
- **Aluno**: Luiz Gustavo Dacome Damas
- **Disciplina**: Estrutura de Dados 2
- **Professor**: Emilio Bergamim Júnior
- **Turma**: Noturno

## ⚙️ Requisitos
- Compilador GCC
- Sistema Windows/Linux
- Imagens no formato PGM P2