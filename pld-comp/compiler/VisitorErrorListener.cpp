#include "VisitorErrorListener.h"

#include "Token.h"
#include "iostream"

using namespace std;

// Variable statique pour indiquer si une erreur a été rencontrée
bool VisitorErrorListener::mHasError = false;

// Ajoute une erreur ou un avertissement avec un message, une ligne et un type d'erreur
void VisitorErrorListener::addError(const string &message, int line,
                                    ErrorType errorType)
{
  // Vérifie le type d'erreur
  switch (errorType)
  {
  case ErrorType::Error:
    cerr << "Error: "; // Préfixe pour une erreur
    mHasError = true;  // Marque qu'une erreur a été rencontrée
    break;
  case ErrorType::Warning:
    cerr << "Warning: "; // Préfixe pour un avertissement
    break;
  }

  // Affiche le message d'erreur ou d'avertissement avec le numéro de ligne
  cerr << "Line " << line << " " << message << endl;
}

// Ajoute une erreur ou un avertissement en utilisant un contexte de règle du parser
void VisitorErrorListener::addError(antlr4::ParserRuleContext *ctx,
                                    const string &message,
                                    ErrorType errorType)
{
  // Récupère la ligne de début du contexte et appelle la méthode précédente
  addError(message, ctx->getStart()->getLine(), errorType);
}

// Ajoute une erreur ou un avertissement avec seulement un message et un type d'erreur
void VisitorErrorListener::addError(const string &message,
                                    ErrorType errorType)
{
  // Vérifie le type d'erreur
  switch (errorType)
  {
  case ErrorType::Error:
    cerr << "Error: "; // Préfixe pour une erreur
    mHasError = true;  // Marque qu'une erreur a été rencontrée
    break;
  case ErrorType::Warning:
    cerr << "Warning: "; // Préfixe pour un avertissement
    break;
  }

  // Affiche le message d'erreur ou d'avertissement
  cerr << message << endl;
}