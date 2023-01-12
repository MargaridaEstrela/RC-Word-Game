# RC-Word-Game
Programming using the Sockets interface - “RC Word Game”
Grupo 34; Turno RC-3L17
Margarida Estrela nº99271
Rui Moniz nº99323

# Organização de Ficheiros

A nossa diretoria de projeto é constituída por várias sub diretorias e ficheiros, estando os mesmos explicados abaixo:
- word_eng : ficheiro de palavras, exatamente igual ao fornecido na página da cadeira;
- constants.hpp : ficheiro de constantes usadas em todo o projeto. A constante TIME_LIMIT gere o timeout do UDP, pelo que esse
pode ser manipulado nos testes feitos;
- aux_functions.cpp : ficheiro de funções auxiliares, usadas tanto no player como no server;
- 2022_2023_proj_auto_avaliacao : ficheiro excel de autoavaliação, certos comentários foram adicionados a certos pontos para esclarecer
certas escolhas;
- reports : diretoria contendo os report.html para todos os testes de servidor, disponibilizados na página da cadeira;
- player app examples : prints de exemplos de interação da nossa aplicação player com o servidor tejo, de forma a demonstrar a sua
funcionalidade;
- playerApp : diretoria contendo o ficheiro player.cpp, que contém o source code da aplicação player;
- gameServer : diretoria contendo todo o source code relacionado à implementação do servidor. O ficheiro server.cpp define o arranque
do servidor; server_udp.cpp define a funcionalidade do servidor, segundo comunicações UDP; server_tcp.cpp define a funcionalidade do
servidor, segundo comunicações TCP; data.cpp define várias funções auxiliares usadas em ambas as vertentes do servidor, nomeadamente
relacionada a interação com ficheiros
- Makefile : Makefile usada para compilar todo o projeto. Revela alguns warnings, mas não suscita qualquer erro de compilação. Após o
comando make, cria na diretoria 'raiz' os executáveis GS (servidor) e player (cliente). Adicionalmente, são criados os executáveis server_udp
e server_tcp, que suportam o GS e, portanto, não é suposto serem executados isoladamente (para testar servidor, usar ./GS word_eng.txt -v). 
Make clean, para além da funcionalidade normal, também remove as diretorias GAMES e SCORE, dando, de certa forma, um reset completo ao servidor.
