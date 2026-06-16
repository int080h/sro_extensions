#include "core/CommandHistory.h"

void CommandHistory::Execute(std::unique_ptr<ICommand> cmd) {
    cmd->Execute();
    m_undo.push_back(std::move(cmd));
    m_redo.clear();
}

void CommandHistory::Undo() {
    if (m_undo.empty()) return;
    auto cmd = std::move(m_undo.back());
    m_undo.pop_back();
    cmd->Undo();
    m_redo.push_back(std::move(cmd));
}

void CommandHistory::Redo() {
    if (m_redo.empty()) return;
    auto cmd = std::move(m_redo.back());
    m_redo.pop_back();
    cmd->Execute();
    m_undo.push_back(std::move(cmd));
}

void CommandHistory::Clear() {
    m_undo.clear();
    m_redo.clear();
}
