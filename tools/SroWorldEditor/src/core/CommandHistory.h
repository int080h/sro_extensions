#pragma once
#include <memory>
#include <vector>
#include <functional>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* Name() const = 0;
};

class CommandHistory {
public:
    void Execute(std::unique_ptr<ICommand> cmd);
    void Undo();
    void Redo();
    bool CanUndo() const { return !m_undo.empty(); }
    bool CanRedo() const { return !m_redo.empty(); }
    void Clear();

private:
    std::vector<std::unique_ptr<ICommand>> m_undo;
    std::vector<std::unique_ptr<ICommand>> m_redo;
};
