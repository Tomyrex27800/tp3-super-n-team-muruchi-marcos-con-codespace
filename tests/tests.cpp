#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include "../hangman.h"

using namespace std;

class HangmanTest : public ::testing::Test {
protected:
    Hangman hangman;
    
    void SetUp() override {
        hangman = Hangman();
    }
    
    void TearDown() override {
    }
};

// Tests para agregar jugadores
TEST_F(HangmanTest, AddPlayerSuccess) {
    EXPECT_TRUE(hangman.addPlayer("Alice"));
    EXPECT_TRUE(hangman.addPlayer("Bob"));
    EXPECT_EQ(hangman.getPlayerCount(), 2);
    
    vector<string> players = hangman.getPlayerNames();
    EXPECT_EQ(players[0], "Alice");
    EXPECT_EQ(players[1], "Bob");
}

TEST_F(HangmanTest, AddPlayerDuplicateName) {
    EXPECT_TRUE(hangman.addPlayer("Alice"));
    EXPECT_FALSE(hangman.addPlayer("Alice")); // Nombre duplicado
    EXPECT_EQ(hangman.getPlayerCount(), 1);
}

TEST_F(HangmanTest, AddPlayerEmptyName) {
    EXPECT_FALSE(hangman.addPlayer("")); // Nombre vacío
    EXPECT_EQ(hangman.getPlayerCount(), 0);
}

TEST_F(HangmanTest, AddPlayerAfterGameStarted) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    EXPECT_TRUE(hangman.startGame());
    
    EXPECT_FALSE(hangman.addPlayer("Charlie")); // No se puede agregar después de iniciar
    EXPECT_EQ(hangman.getPlayerCount(), 2);
}

// Tests para iniciar el juego
TEST_F(HangmanTest, StartGameWithEnoughPlayers) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    
    EXPECT_FALSE(hangman.isGameStarted());
    EXPECT_TRUE(hangman.startGame());
    EXPECT_TRUE(hangman.isGameStarted());
    EXPECT_EQ(hangman.getGameState(), GameState::IN_PROGRESS);
}

TEST_F(HangmanTest, StartGameWithInsufficientPlayers) {
    hangman.addPlayer("Alice");
    
    EXPECT_FALSE(hangman.startGame()); // Solo 1 jugador
    EXPECT_FALSE(hangman.isGameStarted());
    EXPECT_EQ(hangman.getGameState(), GameState::WAITING_FOR_PLAYERS);
}

TEST_F(HangmanTest, StartGameTwice) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    
    EXPECT_TRUE(hangman.startGame());
    EXPECT_FALSE(hangman.startGame()); // No se puede iniciar dos veces
}

// Tests para el estado inicial del juego
TEST_F(HangmanTest, InitialGameState) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    EXPECT_FALSE(hangman.getWordDisplay().empty());
    EXPECT_TRUE(hangman.getGuessedLetters().empty());
    EXPECT_EQ(hangman.getIncorrectGuesses(), 0);
    EXPECT_EQ(hangman.getRemainingGuesses(), 6);
    EXPECT_TRUE(hangman.getWinner().empty());
    EXPECT_FALSE(hangman.isGameFinished());
    
    // Verificar que hay un jugador actual
    EXPECT_FALSE(hangman.getCurrentPlayerName().empty());
}

// Tests para turnos
TEST_F(HangmanTest, PlayerTurns) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string firstPlayer = hangman.getCurrentPlayerName();
    EXPECT_TRUE(hangman.isPlayerTurn(firstPlayer));
    EXPECT_FALSE(hangman.isPlayerTurn("NonExistentPlayer"));
    
    // El otro jugador no debe ser el turno actual
    vector<string> players = hangman.getPlayerNames();
    for (const string& player : players) {
        if (player != firstPlayer) {
            EXPECT_FALSE(hangman.isPlayerTurn(player));
        }
    }
}

TEST_F(HangmanTest, TurnRotation) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string firstPlayer = hangman.getCurrentPlayerName();
    int initialIndex = hangman.getCurrentPlayerIndex();
    
    // Hacer una jugada incorrecta para cambiar de turno
    hangman.guessLetter(firstPlayer, 'Z'); // Asumiendo que Z no está en ninguna palabra
    
    string secondPlayer = hangman.getCurrentPlayerName();
    EXPECT_NE(firstPlayer, secondPlayer);
    EXPECT_NE(initialIndex, hangman.getCurrentPlayerIndex());
}

// Tests para adivinar letras
TEST_F(HangmanTest, GuessLetterCorrect) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    string initialDisplay = hangman.getWordDisplay();
    
    // Intentar con varias letras comunes que probablemente estén en las palabras
    bool foundLetter = false;
    for (char c = 'A'; c <= 'Z' && !foundLetter; ++c) {
        hangman.guessLetter(currentPlayer, c);
        if (hangman.getWordDisplay() != initialDisplay) {
            foundLetter = true;
            EXPECT_TRUE(hangman.getGuessedLetters().find(c) != hangman.getGuessedLetters().end());
        }
    }
}

TEST_F(HangmanTest, GuessLetterWrongTurn) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    vector<string> allPlayers = hangman.getPlayerNames();
    string otherPlayer;
    
    for (const string& player : allPlayers) {
        if (player != currentPlayer) {
            otherPlayer = player;
            break;
        }
    }
    
    EXPECT_FALSE(hangman.guessLetter(otherPlayer, 'A')); // No es su turno
}

TEST_F(HangmanTest, GuessLetterAlreadyGuessed) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    
    EXPECT_TRUE(hangman.guessLetter(currentPlayer, 'A'));
    
    // Cambiar al siguiente jugador y intentar la misma letra
    currentPlayer = hangman.getCurrentPlayerName();
    EXPECT_FALSE(hangman.guessLetter(currentPlayer, 'A')); // Ya fue adivinada
}

TEST_F(HangmanTest, GuessLetterBeforeGameStart) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    
    EXPECT_FALSE(hangman.guessLetter("Alice", 'A')); // Juego no iniciado
}

// Tests para adivinar palabras
TEST_F(HangmanTest, GuessWordCorrect) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    string currentWord = hangman.getCurrentWord();
    
    EXPECT_TRUE(hangman.guessWord(currentPlayer, currentWord));
    EXPECT_TRUE(hangman.isGameFinished());
    EXPECT_EQ(hangman.getWinner(), currentPlayer);
    EXPECT_EQ(hangman.getGameState(), GameState::FINISHED);
}

TEST_F(HangmanTest, GuessWordIncorrect) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    int initialIncorrect = hangman.getIncorrectGuesses();
    
    EXPECT_TRUE(hangman.guessWord(currentPlayer, "WRONGWORD"));
    EXPECT_EQ(hangman.getIncorrectGuesses(), initialIncorrect + 1);
    EXPECT_TRUE(hangman.getWinner().empty()); // No hay ganador aún
    EXPECT_NE(hangman.getCurrentPlayerName(), currentPlayer); // Cambió de turno
}

TEST_F(HangmanTest, GuessWordWrongTurn) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    vector<string> allPlayers = hangman.getPlayerNames();
    string otherPlayer;
    
    for (const string& player : allPlayers) {
        if (player != currentPlayer) {
            otherPlayer = player;
            break;
        }
    }
    
    EXPECT_FALSE(hangman.guessWord(otherPlayer, "ANYWORD")); // No es su turno
}

TEST_F(HangmanTest, GuessWordBeforeGameStart) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    
    EXPECT_FALSE(hangman.guessWord("Alice", "ANYWORD")); // Juego no iniciado
}

// Tests para el límite de intentos incorrectos
TEST_F(HangmanTest, MaxIncorrectGuessesReached) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    // Hacer suficientes intentos incorrectos para terminar el juego
    int maxGuesses = hangman.getRemainingGuesses();
    
    for (int i = 0; i < maxGuesses; ++i) {
        if (hangman.isGameFinished()) break;
        
        string currentPlayer = hangman.getCurrentPlayerName();
        hangman.guessWord(currentPlayer, "WRONGWORD" + to_string(i));
    }
    
    EXPECT_TRUE(hangman.isGameFinished());
    EXPECT_TRUE(hangman.getWinner().empty()); // No hay ganador
    EXPECT_EQ(hangman.getRemainingGuesses(), 0);
}

// Tests para el pool de palabras
TEST_F(HangmanTest, WordFromValidPool) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentWord = hangman.getCurrentWord();
    
    // Verificar que la palabra no está vacía
    EXPECT_FALSE(currentWord.empty());
    
    // Verificar que contiene solo letras mayúsculas
    for (char c : currentWord) {
        EXPECT_TRUE(isalpha(c) && isupper(c));
    }
}

// Tests para case insensitivity
TEST_F(HangmanTest, CaseInsensitiveGuessing) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.startGame();
    
    string currentPlayer = hangman.getCurrentPlayerName();
    
    // Probar con letra minúscula
    EXPECT_TRUE(hangman.guessLetter(currentPlayer, 'a'));
    EXPECT_TRUE(hangman.getGuessedLetters().find('A') != hangman.getGuessedLetters().end());
    
    currentPlayer = hangman.getCurrentPlayerName();
    string currentWord = hangman.getCurrentWord();
    
    // Probar adivinanza de palabra en minúsculas
    string lowerWord = currentWord;
    transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
    
    EXPECT_TRUE(hangman.guessWord(currentPlayer, lowerWord));
    EXPECT_TRUE(hangman.isGameFinished());
    EXPECT_EQ(hangman.getWinner(), currentPlayer);
}

// Test para múltiples jugadores
TEST_F(HangmanTest, MultiplePlayersRotation) {
    hangman.addPlayer("Alice");
    hangman.addPlayer("Bob");
    hangman.addPlayer("Charlie");
    hangman.startGame();
    
    EXPECT_EQ(hangman.getPlayerCount(), 3);
    
    string player1 = hangman.getCurrentPlayerName();
    hangman.guessLetter(player1, 'Z'); // Asumiendo letra incorrecta
    
    string player2 = hangman.getCurrentPlayerName();
    EXPECT_NE(player1, player2);
    hangman.guessLetter(player2, 'Y'); // Asumiendo letra incorrecta
    
    string player3 = hangman.getCurrentPlayerName();
    EXPECT_NE(player2, player3);
    EXPECT_NE(player1, player3);
    hangman.guessLetter(player3, 'X'); // Asumiendo letra incorrecta
    
    string backToPlayer1 = hangman.getCurrentPlayerName();
    EXPECT_EQ(player1, backToPlayer1);
}
