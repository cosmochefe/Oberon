# Compilador para Oberon-0

Este projeto visa implementar em linguagem C um compilador para a linguagem de programação Oberon-0, descrita no livro “Compiler Construction” de Niklaus Wirth. O código está estruturado de forma genérica o suficiente para permitir adaptações a outras linguagens, modificando-se a estrutura do analisador sintático conforme as novas regras gramaticais.

A abordagem de implementação utilizada é a de “análise descendente recursiva” por sua simplicidade e facilidade de entendimento.

Nota: os arquivos de projeto do Xcode estão presentes apenas por conveniência. Todo o código tem por base o padrão C99 e provavelmente pode ser compilado em outros sistemas operacionais além do Mac OS X.