#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "manage_data.h"
#include "pgm_operations.h"

// Compacta arquivo removendo imagens deletadas
void compress_data_file(const char *data_file_name, const char *compress_file_name){
    FILE *data_file = fopen(data_file_name, "rb");
    if(!data_file){
        printf("Erro ao abrir o arquivo de dados para leitura.\n");
        return;
    }
    
    FILE *compress_file = fopen(compress_file_name, "wb");
    if(!compress_file){
        printf("Erro ao abrir o arquivo de compressão para escrita.\n");
        fclose(data_file);
        return;
    }
    
    FILE *index_file = fopen("data/indices.bin", "rb");
    if(!index_file){
        printf("Erro ao abrir arquivo de índices.\n");
        fclose(data_file);
        fclose(compress_file);
        return;
    }
    
    // Criar novo arquivo de índices temporário para preservar nomes
    FILE *new_index_file = fopen("data/indices_temp.bin", "wb");
    if(!new_index_file){
        printf("Erro ao criar arquivo de índices temporário.\n");
        fclose(data_file);
        fclose(compress_file);
        fclose(index_file);
        return;
    }
    
    // Verificar se o arquivo está vazio
    fseek(data_file, 0, SEEK_END);
    long file_size = ftell(data_file);
    if (file_size == 0) {
        printf("Arquivo de dados está vazio.\n");
        fclose(data_file);
        fclose(compress_file);
        fclose(index_file);
        return;
    }
    fseek(data_file, 0, SEEK_SET);
    
    int images_processed = 0;
    int images_kept = 0;
    long new_offset = 0; // Controlar novo offset no arquivo comprimido
    
    // Ler todos os índices
    IndiceRecord reg;
    while(fread(&reg, sizeof(IndiceRecord), 1, index_file)) {
        images_processed++;
        
        // Verificar se a imagem está marcada como removida no índice
        if(!reg.is_removed) {
            // Copiar esta imagem do arquivo de dados
            fseek(data_file, reg.offset, SEEK_SET);
            
            int width, height, grey_levels;
            if(fread(&width, sizeof(int), 1, data_file) != 1 ||
               fread(&height, sizeof(int), 1, data_file) != 1 ||
               fread(&grey_levels, sizeof(int), 1, data_file) != 1) {
                printf("Erro ao ler cabeçalho da imagem no offset %ld\n", reg.offset);
                continue;
            }
            
            // Validar dimensões para evitar corrupção
            if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
                printf("Dimensões inválidas: %dx%d (offset: %ld) - Pulando imagem\n", 
                       width, height, reg.offset);
                continue;
            }
            
            // Posicionar no final do arquivo comprimido e salvar offset atual
            fseek(compress_file, 0, SEEK_END);
            new_offset = ftell(compress_file);
            
            // Escrever cabeçalho no arquivo comprimido
            if (fwrite(&width, sizeof(int), 1, compress_file) != 1 ||
                fwrite(&height, sizeof(int), 1, compress_file) != 1 ||
                fwrite(&grey_levels, sizeof(int), 1, compress_file) != 1) {
                printf("Erro ao escrever cabeçalho\n");
                continue;
            }
            
            // Copiar dados - formato depende do sinal de grey_levels
            int copy_success = 1;
            
            if (grey_levels < 0) {
                // Formato RLE - copiar com primeiro_pixel + counts
                for(int i = 0; i < height && copy_success; i++) {
                    unsigned char primeiro_pixel;
                    if(fread(&primeiro_pixel, sizeof(unsigned char), 1, data_file) != 1) {
                        printf("Erro ao ler primeiro pixel da linha %d\n", i);
                        copy_success = 0;
                        break;
                    }
                    if(fwrite(&primeiro_pixel, sizeof(unsigned char), 1, compress_file) != 1) {
                        printf("Erro ao escrever primeiro pixel da linha %d\n", i);
                        copy_success = 0;
                        break;
                    }
                    
                    // Copiar counts até completar a linha
                    int pixels_lidos = 0;
                    while(pixels_lidos < width && copy_success) {
                        int count;
                        if(fread(&count, sizeof(int), 1, data_file) != 1) {
                            printf("Erro ao ler count na linha %d (pixels_lidos: %d/%d)\n", 
                                   i, pixels_lidos, width);
                            copy_success = 0;
                            break;
                        }
                        
                        // Validar count
                        if (count <= 0 || count > width || pixels_lidos + count > width) {
                            printf("Count inválido: %d na linha %d (pixels_lidos: %d/%d)\n", 
                                   count, i, pixels_lidos, width);
                            copy_success = 0;
                            break;
                        }
                        
                        if(fwrite(&count, sizeof(int), 1, compress_file) != 1) {
                            printf("Erro ao escrever count na linha %d\n", i);
                            copy_success = 0;
                            break;
                        }
                        pixels_lidos += count;
                    }
                    
                    // Verificar se a linha foi completamente processada
                    if (pixels_lidos != width) {
                        printf("Linha %d incompleta: %d/%d pixels\n", i, pixels_lidos, width);
                        copy_success = 0;
                    }
                }
            } else {
                // Formato RAW - copiar pixels diretamente
                long pixels_total = (long)width * height;
                unsigned char *pixel_data = malloc(pixels_total);
                
                if (!pixel_data) {
                    printf("Erro ao alocar memória para pixels RAW\n");
                    copy_success = 0;
                } else {
                    if (fread(pixel_data, 1, pixels_total, data_file) != (size_t)pixels_total) {
                        printf("Erro ao ler pixels RAW\n");
                        copy_success = 0;
                    } else if (fwrite(pixel_data, 1, pixels_total, compress_file) != (size_t)pixels_total) {
                        printf("Erro ao escrever pixels RAW\n");
                        copy_success = 0;
                    }
                    free(pixel_data);
                }
            }
            
            if (copy_success) {
                images_kept++;
                printf("✅ Imagem %d: %dx%d copiada (offset: %ld -> %ld)\n", 
                       images_kept, width, height, reg.offset, new_offset);
                
                // Salvar índice com nome preservado e novo offset
                IndiceRecord new_reg;
                strncpy(new_reg.name, reg.name, max_name - 1);
                new_reg.name[max_name - 1] = '\0';
                new_reg.offset = new_offset;
                new_reg.is_removed = 0;
                fwrite(&new_reg, sizeof(IndiceRecord), 1, new_index_file);
            } else {
                printf("❌ Falha ao copiar imagem %dx%d (offset: %ld)\n", 
                       width, height, reg.offset);
                // Reverter posição no arquivo comprimido
                fseek(compress_file, new_offset, SEEK_SET);
            }
        } else {
            printf("🗑️ Imagem '%s' (offset: %ld) marcada para deleção - Ignorando\n", 
                   reg.name, reg.offset);
        }
    }
    
    fclose(data_file);
    fclose(compress_file);
    fclose(index_file);
    fclose(new_index_file);
    
    printf("Compactação concluída: %d imagens processadas, %d mantidas\n", 
           images_processed, images_kept);
    
    // Substituir índice antigo pelo novo (com nomes preservados)
    remove("data/indices.bin");
    rename("data/indices_temp.bin", "data/indices.bin");
    printf("✅ Índices preservados com nomes originais!\n");
}

// Função auxiliar para calcular o tamanho de uma imagem RLE no arquivo
long calculate_rle_image_size(int width, int height) {
    // Tamanho = cabeçalho (3 ints: width, height, grey_levels) + para cada linha (1 byte primeiro_pixel + counts variáveis)
    // Como não sabemos exatamente quantos counts, vamos estimar o pior caso
    // Para limiarização: máximo de width counts por linha (alternando 0 e grey_levels)
    long size = 3 * sizeof(int); // cabeçalho: width, height, grey_levels (sem is_removed)
    
    for(int i = 0; i < height; i++) {
        size += sizeof(unsigned char); // primeiro_pixel
        // No pior caso, cada pixel seria diferente do anterior (width counts)
        // Na prática, RLE comprime, então estimamos metade dos pixels como counts
        int estimated_counts = (width / 2) + 1; // +1 para segurança
        size += estimated_counts * sizeof(int);
    }
    
    return size;
}

// Função para inserir imagem no topo do arquivo (operação custosa)
void insert_image_at_top(const char *data_base_file_name, PGM *img, long *new_offset) {
    printf("🔄 Iniciando inserção no topo (operação custosa)...\n");
    
    // Primeiro, salvar a nova imagem em um arquivo temporário
    char temp_new_image[] = "data/temp_new_image.bin";
    long temp_offset;
    RLE_compress_v2(img, temp_new_image, &temp_offset);
    
    // Abrir arquivo temporário para ler o tamanho da nova imagem
    FILE *temp_file = fopen(temp_new_image, "rb");
    if (!temp_file) {
        printf("Erro ao criar arquivo temporário!\n");
        *new_offset = -1;
        return;
    }
    
    fseek(temp_file, 0, SEEK_END);
    long new_image_size = ftell(temp_file);
    fseek(temp_file, 0, SEEK_SET);
    
    // Ler o conteúdo da nova imagem
    unsigned char *new_image_data = malloc(new_image_size);
    fread(new_image_data, 1, new_image_size, temp_file);
    fclose(temp_file);
    remove(temp_new_image); // Remover arquivo temporário
    
    // Abrir arquivo original para leitura
    FILE *original_file = fopen(data_base_file_name, "rb");
    unsigned char *original_data = NULL;
    long original_size = 0;
    
    if (original_file) {
        // Ler todo o conteúdo original
        fseek(original_file, 0, SEEK_END);
        original_size = ftell(original_file);
        fseek(original_file, 0, SEEK_SET);
        
        if (original_size > 0) {
            original_data = malloc(original_size);
            fread(original_data, 1, original_size, original_file);
        }
        fclose(original_file);
    }
    
    // Reescrever arquivo: nova imagem + dados originais
    FILE *output_file = fopen(data_base_file_name, "wb");
    if (!output_file) {
        printf("Erro ao abrir arquivo para escrita!\n");
        free(new_image_data);
        if (original_data) free(original_data);
        *new_offset = -1;
        return;
    }
    
    // Escrever nova imagem no início (offset 0)
    *new_offset = 0;
    fwrite(new_image_data, 1, new_image_size, output_file);
    
    // Escrever dados originais após a nova imagem
    if (original_data && original_size > 0) {
        fwrite(original_data, 1, original_size, output_file);
    }
    
    fclose(output_file);
    
    // Liberar memória
    free(new_image_data);
    if (original_data) free(original_data);
    
    printf("✅ Imagem inserida no topo com sucesso!\n");
    printf("📍 Novo offset: %ld, Tamanho: %ld bytes\n", *new_offset, new_image_size);
    
    // IMPORTANTE: Todos os offsets no índice agora estão desatualizados!
    printf("⚠️  ATENÇÃO: Todos os offsets existentes foram deslocados em %ld bytes!\n", new_image_size);
    printf("💡 Recomenda-se recriar o arquivo de índices após esta operação.\n");
}

// Função de append, adiciona uma nova imagem ao arquivo de dados usando RLE
void append_image_to_data_file(const char *data_base_file_name, PGM *img, int top, const char *nome_imagem){
    long offset;
    
    if(top){
        // Salvar nomes existentes antes da inserção no topo
        char existing_names[100][max_name];
        int existing_count = 0;
        
        FILE *old_index = fopen("data/indices.bin", "rb");
        if (old_index) {
            IndiceRecord reg;
            while(fread(&reg, sizeof(IndiceRecord), 1, old_index) && existing_count < 100) {
                if (!reg.is_removed) {
                    strcpy(existing_names[existing_count], reg.name);
                    existing_count++;
                }
            }
            fclose(old_index);
        }
        
        // Implementar inserção no topo (operação custosa)
        insert_image_at_top(data_base_file_name, img, &offset);
        
        if (offset == -1) {
            printf("❌ Erro na inserção no topo!\n");
            return;
        }
        
        printf("✅ Imagem inserida no topo do arquivo!\n");
        printf("📍 Offset: %ld\n", offset);
        
        // Automaticamente recriar índices preservando nomes
        printf("🔄 Recriando índices com preservação de nomes...\n");
        recreate_index_with_names(data_base_file_name, "data/indices.bin", 
                                  nome_imagem, existing_names, existing_count);
        
    } else {
        // Usar a função RLE para inserir no final (comportamento normal)
        RLE_compress_v2(img, data_base_file_name, &offset);
        printf("✅ Imagem adicionada no final do arquivo!\n");
        printf("📍 Offset: %ld\n", offset);
        
        // Adicionar ao índice para inserção no final
        FILE *index = fopen("data/indices.bin", "ab");
        if (index) {
            IndiceRecord reg;
            strncpy(reg.name, nome_imagem, max_name - 1);
            reg.name[max_name - 1] = '\0';
            reg.offset = offset;
            reg.is_removed = 0;
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
            printf("📋 Índice atualizado para '%s'\n", nome_imagem);
        } else {
            printf("❌ Erro ao atualizar índice!\n");
        }
    }
}

// Função para recriar o arquivo de índices após inserção no topo
void recreate_index_file(const char *data_file_name, const char *index_file_name) {
    FILE *data_file = fopen(data_file_name, "rb");
    if (!data_file) {
        printf("❌ Erro ao abrir arquivo de dados!\n");
        return;
    }
    
    // Remover arquivo de índices antigo
    remove(index_file_name);
    
    // Percorrer arquivo de dados e recriar índices
    fseek(data_file, 0, SEEK_END);
    long file_size = ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    
    int image_count = 0;
    printf("📋 Recriando índices...\n");
    
    while(ftell(data_file) < file_size) {
        long current_offset = ftell(data_file);
        int width, height, grey_levels;
        
        // Ler cabeçalho (sem flag is_removed)
        if(fread(&width, sizeof(int), 1, data_file) != 1 ||
           fread(&height, sizeof(int), 1, data_file) != 1 ||
           fread(&grey_levels, sizeof(int), 1, data_file) != 1) {
            break;
        }
        
        image_count++;
        
        // Gerar nome automático para a imagem
        char auto_name[100];
        snprintf(auto_name, sizeof(auto_name), "auto_img_%d_%dx%d", 
                image_count, width, height);
        
        printf("   %d. %s (offset: %ld) [ATIVA]\n", 
               image_count, auto_name, current_offset);
        
        // Adicionar ao índice (assumir como ativa)
        FILE *index = fopen(index_file_name, "ab");
        if (index) {
            IndiceRecord reg;
            strncpy(reg.name, auto_name, max_name - 1);
            reg.name[max_name - 1] = '\0';
            reg.offset = current_offset;
            reg.is_removed = 0; // Marcar como ativa
            
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
        }
        
        // Pular dados RLE da imagem
        for(int i = 0; i < height; i++) {
            unsigned char primeiro_pixel;
            fread(&primeiro_pixel, sizeof(unsigned char), 1, data_file);
            
            int pixels_lidos = 0;
            while(pixels_lidos < width) {
                int count;
                if(fread(&count, sizeof(int), 1, data_file) != 1) {
                    goto end_recreation;
                }
                pixels_lidos += count;
            }
        }
    }
    
end_recreation:
    fclose(data_file);
    printf("✅ Índices recriados! Total de imagens: %d\n", image_count);
    printf("💡 Use nomes automáticos gerados para acessar as imagens.\n");
}

// Nova função para recriar índices preservando nomes após inserção no topo
void recreate_index_with_names(const char *data_file_name, const char *index_file_name, 
                               const char *new_image_name, char existing_names[][max_name], 
                               int existing_count) {
    FILE *data_file = fopen(data_file_name, "rb");
    if (!data_file) {
        printf("❌ Erro ao abrir arquivo de dados!\n");
        return;
    }
    
    // Remover arquivo de índices antigo
    remove(index_file_name);
    
    // Percorrer arquivo de dados e recriar índices
    fseek(data_file, 0, SEEK_END);
    long file_size = ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    
    int image_count = 0;
    printf("Recriando índices preservando nomes...\n");
    
    while(ftell(data_file) < file_size) {
        long current_offset = ftell(data_file);
        int width, height, grey_levels;
        
        // Ler cabeçalho
        if(fread(&width, sizeof(int), 1, data_file) != 1 ||
           fread(&height, sizeof(int), 1, data_file) != 1 ||
           fread(&grey_levels, sizeof(int), 1, data_file) != 1) {
            break;
        }
        
        image_count++;
        
        // Determinar nome: primeira imagem usa novo nome, demais usam nomes preservados
        char final_name[max_name];
        if (image_count == 1) {
            // Primeira imagem é a nova inserida no topo
            strncpy(final_name, new_image_name, max_name - 1);
            final_name[max_name - 1] = '\0';
        } else if (image_count - 2 < existing_count) {
            // Usar nome original preservado (image_count-2 porque primeira é nova)
            strcpy(final_name, existing_names[image_count - 2]);
        } else {
            // Fallback para nome automático se necessário
            snprintf(final_name, sizeof(final_name), "auto_img_%d_%dx%d", 
                    image_count, width, height);
        }
        
        printf("   %d. %s (offset: %ld) [ATIVA]\n", 
               image_count, final_name, current_offset);
        
        // Adicionar ao índice
        FILE *index = fopen(index_file_name, "ab");
        if (index) {
            IndiceRecord reg;
            strncpy(reg.name, final_name, max_name - 1);
            reg.name[max_name - 1] = '\0';
            reg.offset = current_offset;
            reg.is_removed = 0;
            
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
        }
        
        // Pular dados da imagem (RLE ou RAW)
        if (grey_levels < 0) {
            // Formato RLE (grey_levels negativo)
            for(int i = 0; i < height; i++) {
                unsigned char primeiro_pixel;
                fread(&primeiro_pixel, sizeof(unsigned char), 1, data_file);
                
                int pixels_lidos = 0;
                while(pixels_lidos < width) {
                    int count;
                    if(fread(&count, sizeof(int), 1, data_file) != 1) {
                        goto end_recreation_with_names;
                    }
                    pixels_lidos += count;
                }
            }
        } else {
            // Formato RAW (grey_levels positivo) - pular pixels diretamente
            long pixels_size = (long)width * height;
            fseek(data_file, pixels_size, SEEK_CUR);
        }
    }
    
end_recreation_with_names:
    fclose(data_file);
    printf("Índices recriados preservando nomes! Total: %d imagens\n", image_count);
    printf("Nova imagem '%s' inserida no início\n", new_image_name);
    if (existing_count > 0) {
        printf(" %d nomes originais preservados\n", existing_count);
    }
}

// Função para recriar índices após compactação (apenas imagens ativas)
void recreate_index_after_compression(const char *data_file_name, const char *index_file_name) {
    FILE *data_file = fopen(data_file_name, "rb");
    if (!data_file) {
        printf("Erro ao abrir arquivo de dados!\n");
        return;
    }
    
    // Remover arquivo de índices antigo
    remove(index_file_name);
    
    // Percorrer arquivo de dados e recriar índices apenas para imagens ativas
    fseek(data_file, 0, SEEK_END);
    long file_size = ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    
    int image_count = 0;
    printf("Recriando índices após compactação...\n");
    
    while(ftell(data_file) < file_size) {
        long current_offset = ftell(data_file);
        int width, height, grey_levels;
        
        // Ler cabeçalho (sem flag is_removed)
        if(fread(&width, sizeof(int), 1, data_file) != 1 ||
           fread(&height, sizeof(int), 1, data_file) != 1 ||
           fread(&grey_levels, sizeof(int), 1, data_file) != 1) {
            break;
        }
        
        image_count++;
        
        // Gerar nome automático para a imagem
        char auto_name[100];
        snprintf(auto_name, sizeof(auto_name), "img_%d_%dx%d", 
                image_count, width, height);
        
        printf("   %d. %s (offset: %ld) [ATIVA]\n", 
               image_count, auto_name, current_offset);
        
        // Adicionar ao índice (todas são ativas após compactação)
        FILE *index = fopen(index_file_name, "ab");
        if (index) {
            IndiceRecord reg;
            strncpy(reg.name, auto_name, max_name - 1);
            reg.name[max_name - 1] = '\0';
            reg.offset = current_offset;
            reg.is_removed = 0; // Todas são ativas após compactação
            
            fwrite(&reg, sizeof(IndiceRecord), 1, index);
            fclose(index);
        }
        
        // Pular dados da imagem (RLE ou RAW)
        if (grey_levels < 0) {
            // Formato RLE (grey_levels negativo)
            for(int i = 0; i < height; i++) {
                unsigned char primeiro_pixel;
                fread(&primeiro_pixel, sizeof(unsigned char), 1, data_file);
                
                int pixels_lidos = 0;
                while(pixels_lidos < width) {
                    int count;
                    if(fread(&count, sizeof(int), 1, data_file) != 1) {
                        goto end_recreation_compression;
                    }
                    pixels_lidos += count;
                }
            }
        } else {
            // Formato RAW (grey_levels positivo) - pular pixels diretamente
            long pixels_size = (long)width * height;
            fseek(data_file, pixels_size, SEEK_CUR);
        }
    }
    
end_recreation_compression:
    fclose(data_file);
    printf(" Índices recriados após compactação! Total de imagens ativas: %d\n", image_count);
}


