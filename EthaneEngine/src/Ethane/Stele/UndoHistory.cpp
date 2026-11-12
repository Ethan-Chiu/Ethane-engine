#include "ethpch.h"
#include "Ethane/Stele/UndoHistory.h"

namespace Ethane {

	void UndoHistory::Record(const ChangeSet& changes)
	{
		if (changes.Ops.empty())
			return;
		m_Redo.clear();

		if (changes.MergeKey != 0 && !m_Undo.empty() && m_Undo.back().MergeKey == changes.MergeKey)
		{
			// Undoing the merged step undoes the newest change first
			std::vector<SteleOperation>& ops = m_Undo.back().Ops;
			ops.insert(ops.begin(), changes.Inverse.begin(), changes.Inverse.end());
			return;
		}

		m_Undo.push_back({ changes.Label, changes.MergeKey, changes.Inverse });
		if (m_Undo.size() > m_MaxSteps)
			m_Undo.pop_front();
	}

	const std::string& UndoHistory::UndoLabel() const
	{
		static const std::string none;
		return m_Undo.empty() ? none : m_Undo.back().Label;
	}

	const std::string& UndoHistory::RedoLabel() const
	{
		static const std::string none;
		return m_Redo.empty() ? none : m_Redo.back().Label;
	}

	CommitResult UndoHistory::Undo(Stele& stele) { return Replay(stele, m_Undo, m_Redo); }
	CommitResult UndoHistory::Redo(Stele& stele) { return Replay(stele, m_Redo, m_Undo); }

	CommitResult UndoHistory::Replay(Stele& stele, std::deque<Step>& from, std::deque<Step>& to)
	{
		if (from.empty())
			return { .Ok = false, .Error = "nothing to replay" };

		Step step = std::move(from.back());
		from.pop_back();

		Transaction transaction(step.Label, stele.Registry());
		for (SteleOperation& op : step.Ops)
			transaction.Append(std::move(op));

		CommitResult result = stele.Commit(std::move(transaction));
		if (result && result.Changes)
			// The inverse of what was just applied is the way back
			to.push_back({ step.Label, 0, result.Changes->Inverse });
		return result;
	}

	void UndoHistory::Clear()
	{
		m_Undo.clear();
		m_Redo.clear();
	}

}
