#!/bin/bash

# Couleurs pour le formatage
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Compteurs de tests
TESTS_PASSED=0
TESTS_FAILED=0

# Nettoyer les fichiers temporaires avant de commencer
cleanup() {
    rm -f test_out.txt input.txt test.txt error.txt output.txt test.sh
    rm -f bash_output bash_error minishell_output minishell_error
}

# Exécuter avant chaque test
setup() {
    cleanup
}

# Fonction pour comparer la sortie du minishell avec celle de bash --posix
test_command() {
    local test_name="$1"
    local command="$2"
    
    setup

    # Créer les fichiers nécessaires pour le test
    if [[ "$test_name" == *"Input redirection"* ]]; then
        echo "test content" > input.txt
    fi
    
    # Exécuter la commande avec bash --posix
    echo "$command" | bash --posix > bash_output 2>bash_error
    local bash_status=$?
    
    # Exécuter la commande avec minishell
    echo "$command" | ./minishell > minishell_output 2>minishell_error
    local minishell_status=$?
    
    # Comparer les sorties et les statuts
    if diff bash_output minishell_output >/dev/null && 
       diff bash_error minishell_error >/dev/null &&
       [ $bash_status -eq $minishell_status ]; then
        echo -e "${GREEN}[OK]${NC} $test_name"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}[KO]${NC} $test_name"
        echo "Command: $command"
        echo "Expected output:"
        cat bash_output
        echo "Got output:"
        cat minishell_output
        if [ -s bash_error ]; then
            echo "Expected error:"
            cat bash_error
        fi
        if [ -s minishell_error ]; then
            echo "Got error:"
            cat minishell_error
        fi
        echo "Expected status: $bash_status"
        echo "Got status: $minishell_status"
        ((TESTS_FAILED++))
    fi
}

# Test des commandes simples (un test à la fois pour mieux localiser les problèmes)
echo "Testing simple commands..."
test_command "Simple echo" "echo hello world"
test_command "Echo with option" "echo -n test"
test_command "Simple ls" "ls -1"  # -1 pour un format plus stable

# Test des redirections (un à la fois)
echo -e "\nTesting redirections..."
test_command "Simple output redirection" "echo hello > test_out.txt && cat test_out.txt"
test_command "Simple input redirection" "echo sample > input.txt && cat < input.txt"
test_command "Simple append redirection" "echo first > test.txt && echo second >> test.txt && cat test.txt"
test_command "Simple error redirection" "ls nonexistentfile 2> error.txt"

# Test des pipes simples
echo -e "\nTesting pipes..."
test_command "Simple pipe" "echo hello | cat"
test_command "Simple grep pipe" "echo hello | grep hello"

# Test des opérateurs logiques simples
echo -e "\nTesting logical operators..."
test_command "Simple AND success" "true && echo ok"
test_command "Simple AND failure" "false && echo nope"
test_command "Simple OR success" "false || echo ok"
test_command "Simple OR skip" "true || echo nope"

# Test des séquences simples
echo -e "\nTesting command sequences..."
test_command "Two command sequence" "echo a ; echo b"

# Test des variables d'environnement
echo -e "\nTesting environment variables..."
test_command "Print env variable" "FOO=bar env | grep FOO"

# Test des built-ins
echo -e "\nTesting built-ins..."
test_command "Echo builtin" "echo test"
test_command "CD builtin" "cd /tmp && pwd"
test_command "Exit builtin with status" "exit 42 ; echo never_printed"

# Nettoyage final
cleanup

# Résumé des tests
echo -e "\nTest Summary:"
echo "Tests passed: $TESTS_PASSED"
echo "Tests failed: $TESTS_FAILED"
echo "Total tests: $((TESTS_PASSED + TESTS_FAILED))"

# Sortir avec un code d'erreur si des tests ont échoué
[ $TESTS_FAILED -eq 0 ]
