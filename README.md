# 🧵 PingPongOS — Sistemas Operacionais

Projeto desenvolvido como parte da avaliação da disciplina de **Sistemas Operacionais** no curso de **Engenharia da Computação** da **UTFPR**.

---

## 📌 Sobre o Projeto

Este projeto consiste na implementação de partes de um sistema operacional educacional, chamado **PingPongOS**. O principal objetivo é **exercitar conceitos fundamentais de sistemas operacionais**, incluindo:

- Gerenciamento de tarefas
- Escalonamento
- Sincronização
- Preempção
- Gerenciamento de disco

Os códigos foram desenvolvidos com base no material fornecido pelo professor **Marco Aurélio Wehrmeister** e complementados conforme as atividades propostas em aula.

---

## 📁 Estrutura do Projeto

```
PingPongOS/
├── src/              # Códigos-fonte das implementações e testes
├── include/          # Arquivos de cabeçalho organizados por módulo
├── objects/          # Arquivos objeto gerados na compilação
├── objectsbyProf/    # Versão alternativa de objetos fornecidos
├── resultados/       # Saídas de execução dos testes
├── disk.dat          # Arquivo de simulação do disco
├── Makefile          # Script de compilação e execução dos testes
```

---

## ⚙️ Como Compilar e Executar

Use o comando `make` seguido do nome do alvo para compilar e rodar os testes:

```bash
make <alvo>
```

🔹 **Exemplo**: Para rodar o teste do disco:

```bash
make disc1
```

A saída do programa será exibida no terminal. Os comandos de compilação são ocultados para facilitar a leitura.

---

## 🙌 Créditos

- 🧠 Código base disponibilizado pelo **Prof. Marco Aurélio Wehrmeister**
- 🧱 Estrutura e inspiração baseadas no material do **Prof. Carlos A. Maziero (DINF/UFPR)**

---

## 🔗 Mais Informações

Para explicações detalhadas sobre o funcionamento do código base e as atividades propostas, consulte o Moodle da disciplina:

📺 [Explicação no Moodle (YouTube)](https://www.youtube.com/watch?v=K9-FcJdXVEw&feature=youtu.be)
