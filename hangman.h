#ifndef HANGMAN_H
#define HANGMAN_H

#include <string>
#include <vector>
#include <set>
#include <random>

enum class GameState {
    WAITING_FOR_PLAYERS,
    IN_PROGRESS,
    FINISHED
};

enum class TurnAction {
    GUESS_LETTER,
    GUESS_WORD
};

struct Player {
    std::string name;
    bool isActive;
    int points;
    
    Player(const std::string& playerName) : name(playerName), isActive(true), points(0) {}
};

class Hangman {
private:
    std::vector<Player> players;
    std::string currentWord;
    std::string wordDisplay;
    std::set<char> guessedLetters;
    int currentPlayerIndex;
    GameState gameState;
    std::string winner;
    int maxIncorrectGuesses;
    int incorrectGuesses;
    
    std::vector<std::string> wordPool = {
        "PROCESO", "HILO", "SEMAFORO", "MUTEX", "DEADLOCK",
        "PROTOCOLO", "TCP", "UDP", "HTTP", "SOCKET",
        "PIPE", "FIFO", "MEMORIA", "COMPARTIDA", "SINCRONIZACION",
        "KERNEL", "SCHEDULER", "INTERRUPT", "BUFFER", "CACHE",
        "NETWORK", "ROUTING", "PACKET", "ETHERNET", "FIREWALL",
        "THREAD", "FORK", "EXEC", "SIGNAL", "HANDLER",
        "MONITOR", "PRODUCER", "CONSUMER", "BARRIER", "SPINLOCK",
        "QUEUE", "STACK", "HEAP", "VIRTUAL", "PHYSICAL",
        "PAGING", "SWAPPING", "CONTEXT", "SWITCH", "PRIORITY"
    };
    
    std::mt19937 rng;
    
    void selectRandomWord();
    void initializeWordDisplay();
    bool isWordGuessed() const;
    void updateWordDisplay(char letter);
    
public:
    Hangman();
    
    // Gestión de jugadores
    bool addPlayer(const std::string& playerName);
    std::vector<std::string> getPlayerNames() const;
    int getPlayerCount() const;
    std::string getCurrentPlayerName() const;
    void removePlayer(const std::string& playerName);
    std::vector<std::pair<std::string, int>> getPlayersWithPoints() const;
    
    // Control del juego
    bool startGame();
    bool isGameStarted() const;
    bool isGameFinished() const;
    GameState getGameState() const;
    void resetGame();
    void resetToWaiting();
    
    // Jugabilidad
    bool guessLetter(const std::string& playerName, char letter);
    bool guessWord(const std::string& playerName, const std::string& word);
    bool isPlayerTurn(const std::string& playerName) const;
    
    // Información del juego
    std::string getWordDisplay() const;
    std::set<char> getGuessedLetters() const;
    std::string getWinner() const;
    int getRemainingGuesses() const;
    int getIncorrectGuesses() const;
    std::string getCurrentWord() const; // Solo para testing
    
    // Turnos
    void nextTurn();
    int getCurrentPlayerIndex() const;
};

#endif // HANGMAN_H
