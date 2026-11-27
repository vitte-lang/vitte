export interface CallableSignatureFact {
  name: string;
  parameters: readonly string[];
  returnType: string;
  documentation: string;
}

/** Builtin callable signatures listed by compiler/builtin/README.md. */
export const COMPILER_BUILTINS: readonly CallableSignatureFact[] = [
  { name: "print", parameters: ["value: string"], returnType: "void", documentation: "Écrit une chaîne sur la sortie standard." },
  { name: "println", parameters: ["value: string"], returnType: "void", documentation: "Écrit une chaîne suivie d’un saut de ligne." },
  { name: "eprint", parameters: ["value: string"], returnType: "void", documentation: "Écrit une chaîne sur la sortie d’erreur." },
  { name: "eprintln", parameters: ["value: string"], returnType: "void", documentation: "Écrit une chaîne suivie d’un saut de ligne sur la sortie d’erreur." },
  { name: "panic", parameters: ["message: string"], returnType: "never", documentation: "Arrête le programme avec un message d’erreur." },
  { name: "assert", parameters: ["condition: bool"], returnType: "void", documentation: "Arrête le programme si la condition est fausse." },
  { name: "len", parameters: ["value: string"], returnType: "usize", documentation: "Retourne la longueur de la chaîne." },
  { name: "to_string", parameters: ["value: int"], returnType: "string", documentation: "Convertit un entier en chaîne." },
  { name: "type_name", parameters: ["value: error"], returnType: "string", documentation: "Retourne le nom du type d’erreur." },
];
