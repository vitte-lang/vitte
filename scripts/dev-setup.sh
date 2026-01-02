#!/bin/bash
# Development Environment Setup Script
# Sets up a complete development environment for Vitte

set -e

echo "=== Vitte Development Environment Setup ==="
echo ""

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect OS
OS_TYPE=$(uname -s)

echo -e "${BLUE}Detected OS: $OS_TYPE${NC}"
echo ""

# Install system dependencies
install_dependencies() {
    case $OS_TYPE in
        Linux)
            echo -e "${BLUE}Installing Linux dependencies...${NC}"
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y \
                    build-essential \
                    cmake \
                    git \
                    curl \
                    wget \
                    pkg-config \
                    libssl-dev \
                    zlib1g-dev
            elif command -v yum &> /dev/null; then
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y \
                    cmake \
                    git \
                    curl \
                    openssl-devel \
                    zlib-devel
            fi
            ;;
        Darwin)
            echo -e "${BLUE}Installing macOS dependencies...${NC}"
            if ! command -v brew &> /dev/null; then
                echo "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            brew install cmake git curl wget openssl zlib
            ;;
        MINGW*|MSYS*|CYGWIN*)
            echo -e "${YELLOW}Windows detected. Please install:${NC}"
            echo "1. Visual Studio with C++ tools (or MinGW-w64)"
            echo "2. CMake from https://cmake.org/"
            echo "3. Git from https://git-scm.com/"
            ;;
        *)
            echo -e "${YELLOW}Unknown OS: $OS_TYPE${NC}"
            ;;
    esac
    echo -e "${GREEN}✓ System dependencies installed${NC}"
    echo ""
}

# Setup Git hooks
setup_git_hooks() {
    echo -e "${BLUE}Setting up Git hooks...${NC}"

    # Pre-commit hook
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
echo "Running pre-commit checks..."
echo "No automated checks configured."
EOF

    chmod +x .git/hooks/pre-commit
    echo -e "${GREEN}✓ Git hooks configured${NC}"
    echo ""
}

# Initialize submodules
init_submodules() {
    echo -e "${BLUE}Initializing submodules...${NC}"
    git submodule update --init --recursive
    echo -e "${GREEN}✓ Submodules initialized${NC}"
    echo ""
}

# Bootstrap compiler
bootstrap_compiler() {
    echo -e "${BLUE}Bootstrapping compiler...${NC}"
    echo "This may take a while on first run..."

    ./scripts/bootstrap_stage0.sh
    ./scripts/self_host_stage1.sh

    # Verify
    if ./target/debug/vittec --version > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Compiler bootstrapped successfully${NC}"
        ./target/debug/vittec --version
    else
        echo -e "${YELLOW}⚠ Compiler bootstrap may need manual review${NC}"
    fi
    echo ""
}

# Verify installation
verify_setup() {
    echo -e "${BLUE}Verifying setup...${NC}"

    local FAILURES=0

    # Check C compiler
    if command -v gcc &> /dev/null || command -v clang &> /dev/null || command -v cl &> /dev/null; then
        echo -e "${GREEN}✓ C compiler${NC}"
    else
        echo -e "${YELLOW}✗ C compiler not found${NC}"
        FAILURES=$((FAILURES + 1))
    fi

    # Check CMake
    if command -v cmake &> /dev/null; then
        echo -e "${GREEN}✓ CMake${NC}"
    else
        echo -e "${YELLOW}✗ CMake not found${NC}"
        FAILURES=$((FAILURES + 1))
    fi

    # Check Git
    if command -v git &> /dev/null; then
        echo -e "${GREEN}✓ Git${NC}"
    else
        echo -e "${YELLOW}✗ Git not found${NC}"
        FAILURES=$((FAILURES + 1))
    fi

    echo ""
    if [ $FAILURES -eq 0 ]; then
        echo -e "${GREEN}All checks passed!${NC}"
        return 0
    else
        echo -e "${YELLOW}Some checks failed. Please review.${NC}"
        return 1
    fi
}

# Main setup flow
main() {
    install_dependencies
    init_submodules
    setup_git_hooks
    bootstrap_compiler
    verify_setup

    echo ""
    echo -e "${GREEN}=== Setup Complete! ===${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Read the contributor guide: ./CONTRIBUTING.md"
    echo "2. Check out a feature branch: git checkout -b feature/my-feature"
    echo "3. Build: make build"
    echo "4. Test: make test"
    echo "5. Submit a PR!"
    echo ""
    echo "For more info: https://github.com/vitte-lang/vitte"
}

main "$@"
