#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enum for token types
typedef enum {
    StartTag,
    EndTag,
    Text
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* data; // Tag name or text content
} Token;

// Simple DOM Node structure
typedef struct Node {
    char* name; // Tag name or text content
    struct Node** children; // Array of child nodes
    int child_count;
    int child_capacity;
    int is_text; // Distinguish text nodes from elements
} Node;

// Create a new node
Node* create_node(const char* name, int is_text) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->name = strdup(name); // Copy the string
    node->child_count = 0;
    node->child_capacity = 4; // Initial capacity
    node->children = (Node**)malloc(node->child_capacity * sizeof(Node*));
    node->is_text = is_text;
    return node;
}

// Add a child node
void add_child(Node* parent, Node* child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = (Node**)realloc(parent->children, parent->child_capacity * sizeof(Node*));
    }
    parent->children[parent->child_count++] = child;
}

// Print the tree (for debugging)
void print_tree(const Node* node, int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
    if (node->is_text) {
        printf("Text: \"%s\"\n", node->name);
    } else {
        printf("<%s>\n", node->name);
    }
    for (int i = 0; i < node->child_count; i++) {
        print_tree(node->children[i], depth + 1);
    }
    if (!node->is_text && depth > 0) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("</%s>\n", node->name);
    }
}

// Free a node and its children
void free_node(Node* node) {
    if (!node) return;
    free(node->name);
    for (int i = 0; i < node->child_count; i++) {
        free_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

// Tokenizer function
void tokenize(const char* html, Token** tokens, int* token_count, int* token_capacity) {
    *token_count = 0;
    size_t pos = 0;
    char buffer[1024] = {0}; // Buffer for text content
    int buffer_pos = 0;

    while (pos < strlen(html)) {
        char c = html[pos];

        if (c == '<') {
            // Flush any buffered text
            if (buffer_pos > 0) {
                buffer[buffer_pos] = '\0';
                if (*token_count >= *token_capacity) {
                    *token_capacity *= 2;
                    *tokens = (Token*)realloc(*tokens, *token_capacity * sizeof(Token));
                }
                (*tokens)[*token_count].type = Text;
                (*tokens)[*token_count].data = strdup(buffer);
                (*token_count)++;
                buffer_pos = 0;
                buffer[0] = '\0';
            }

            // Check if it's an end tag
            int is_end_tag = (pos + 1 < strlen(html) && html[pos + 1] == '/');
            pos += (is_end_tag ? 2 : 1);

            // Extract tag name
            char tag_name[256] = {0};
            int tag_pos = 0;
            while (pos < strlen(html) && html[pos] != '>') {
                tag_name[tag_pos++] = html[pos++];
            }
            tag_name[tag_pos] = '\0';
            pos++; // Skip '>'

            if (*token_count >= *token_capacity) {
                *token_capacity *= 2;
                *tokens = (Token*)realloc(*tokens, *token_capacity * sizeof(Token));
            }
            (*tokens)[*token_count].type = is_end_tag ? EndTag : StartTag;
            (*tokens)[*token_count].data = strdup(tag_name);
            (*token_count)++;
        } else {
            // Accumulate text content
            buffer[buffer_pos++] = c;
            pos++;
        }
    }

    // Flush remaining text
    if (buffer_pos > 0) {
        buffer[buffer_pos] = '\0';
        if (*token_count >= *token_capacity) {
            *token_capacity *= 2;
            *tokens = (Token*)realloc(*tokens, *token_capacity * sizeof(Token));
        }
        (*tokens)[*token_count].type = Text;
        (*tokens)[*token_count].data = strdup(buffer);
        (*token_count)++;
    }
}

// Build DOM tree from tokens
Node* build_dom(Token* tokens, int token_count) {
    Node* root = create_node("document", 0);
    Node** stack = (Node**)malloc(128 * sizeof(Node*)); // Stack of open elements
    int stack_size = 0;
    stack[stack_size++] = root;

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == StartTag) {
            Node* node = create_node(tokens[i].data, 0);
            add_child(stack[stack_size - 1], node);
            stack[stack_size++] = node;
        } else if (tokens[i].type == EndTag) {
            if (stack_size > 0) stack_size--; // Close the current element
        } else if (tokens[i].type == Text) {
            Node* text_node = create_node(tokens[i].data, 1);
            add_child(stack[stack_size - 1], text_node);
        }
    }

    free(stack);
    return root;
}

int main() {
    // Test HTML string
    const char* html = "<html><body><div>Hello <b>world</b></div></body></html>";

    // Step 1: Tokenize
    int token_capacity = 16;
    int token_count = 0;
    Token* tokens = (Token*)malloc(token_capacity * sizeof(Token));
    tokenize(html, &tokens, &token_count, &token_capacity);

    printf("Tokens:\n");
    for (int i = 0; i < token_count; i++) {
        printf("%s: %s\n",
               tokens[i].type == StartTag ? "StartTag" :
               tokens[i].type == EndTag ? "EndTag" : "Text",
               tokens[i].data);
    }

    // Step 2: Build DOM tree
    Node* dom = build_dom(tokens, token_count);

    // Step 3: Print the tree
    printf("\nDOM Tree:\n");
    print_tree(dom, 0);

    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i].data);
    }
    free(tokens);
    free_node(dom);

    return 0;
}
