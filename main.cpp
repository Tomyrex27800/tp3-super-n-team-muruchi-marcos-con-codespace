#include <iostream>
#include <string>
#include "hangman.h"

using namespace std;

void printGameState(const Hangman& game) {
    cout << "\n=== Estado del Juego ===" << endl;
    cout << "Palabra: " << game.getWordDisplay() << endl;
    cout << "Intentos restantes: " << game.getRemainingGuesses() << endl;
    cout << "Letras adivinadas: ";
    for (char c : game.getGuessedLetters()) {
        cout << c << " ";
    }
    cout << endl;
    if (!game.isGameFinished()) {
        cout << "Turno de: " << game.getCurrentPlayerName() << endl;
    }
    cout << "========================\n" << endl;
}

int main() {
    cout << "=== JUEGO DEL AHORCADO ===" << endl;
    cout << "Temática: Sistemas Operativos, Redes, IPC, Sincronización\n" << endl;
    
    Hangman game;
    
    // Agregar jugadores
    cout << "Agregando jugadores..." << endl;
    game.addPlayer("Alice");
    game.addPlayer("Bob");
    game.addPlayer("Charlie");
    
    cout << "Jugadores agregados: ";
    for (const string& player : game.getPlayerNames()) {
        cout << player << " ";
    }
    cout << "\n" << endl;
    
    // Iniciar el juego
    if (game.startGame()) {
        cout << "¡Juego iniciado exitosamente!" << endl;
        cout << "Palabra secreta (para debug): " << game.getCurrentWord() << endl;
        
        printGameState(game);
        
        // Simulación de algunas jugadas
        cout << "=== Simulando algunas jugadas ===" << endl;
        
        // Alice adivina una letra
        string currentPlayer = game.getCurrentPlayerName();
        cout << currentPlayer << " adivina la letra 'O'..." << endl;
        game.guessLetter(currentPlayer, 'O');
        printGameState(game);
        
        // Bob adivina una letra
        currentPlayer = game.getCurrentPlayerName();
        cout << currentPlayer << " adivina la letra 'E'..." << endl;
        game.guessLetter(currentPlayer, 'E');
        printGameState(game);
        
        // Charlie intenta adivinar la palabra completa
        currentPlayer = game.getCurrentPlayerName();
        cout << currentPlayer << " intenta adivinar la palabra completa: '" << game.getCurrentWord() << "'..." << endl;
        game.guessWord(currentPlayer, game.getCurrentWord());
        printGameState(game);
        
        // Verificar si el juego terminó
        if (game.isGameFinished()) {
            string winner = game.getWinner();
            if (!winner.empty()) {
                cout << "🎉 ¡" << winner << " ha ganado el juego!" << endl;
                cout << "La palabra era: " << game.getCurrentWord() << endl;
            } else {
                cout << "😞 Nadie ganó. Se agotaron los intentos." << endl;
                cout << "La palabra era: " << game.getCurrentWord() << endl;
            }
        }
        
    } else {
        cout << "❌ Error: No se pudo iniciar el juego" << endl;
        cout << "Se necesitan al menos 2 jugadores." << endl;
    }
    
    return 0;
}
