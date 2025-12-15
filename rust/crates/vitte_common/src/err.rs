#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VitteErr {
  InvalidArg,
  Parse,
  Oom,
  Internal,
  AbiMismatch,
}

impl VitteErr {
  pub fn code(self) -> i32 {
    match self {
      VitteErr::InvalidArg => 1,
      VitteErr::Parse => 2,
      VitteErr::Oom => 3,
      VitteErr::Internal => 100,
      VitteErr::AbiMismatch => 101,
    }
  }
}
