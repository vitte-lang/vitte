package tree_sitter_vitte_test

import (
	"testing"

	tree_sitter "github.com/smacker/go-tree-sitter"
	"github.com/tree-sitter/tree-sitter-vitte"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_vitte.Language())
	if language == nil {
		t.Errorf("Error loading Vitte grammar")
	}
}
