#pragma once

#include "Ethane/Brick/Brick.h"

#include <deque>

namespace Ethane {

	// Undo / redo for the changes one user made to one brick. Undoing commits the recorded inverse ops as
	// a new transaction, so the brick only ever moves forward and other subscribers see undo as an
	// ordinary change.
	//
	// Record each of your own commits; consecutive commits with the same non-zero MergeKey become a single
	// step (use a new key for each gesture, e.g. each slider drag).
	class UndoHistory
	{
	public:
		explicit UndoHistory(size_t maxSteps = 256) : m_MaxSteps(maxSteps) {}

		void Record(const ChangeSet& changes);

		bool CanUndo() const { return !m_Undo.empty(); }
		bool CanRedo() const { return !m_Redo.empty(); }
		const std::string& UndoLabel() const; // empty if there's nothing to undo
		const std::string& RedoLabel() const;

		// A step whose ops no longer apply (e.g. someone else deleted the object) is dropped and the
		// error returned
		CommitResult Undo(Brick& brick);
		CommitResult Redo(Brick& brick);

		void Clear();

	private:
		struct Step
		{
			std::string Label;
			uint64_t MergeKey = 0;
			std::vector<BrickOperation> Ops; // what to commit to undo (or redo) this step
		};

		CommitResult Replay(Brick& brick, std::deque<Step>& from, std::deque<Step>& to);

		size_t m_MaxSteps;
		std::deque<Step> m_Undo;
		std::deque<Step> m_Redo;
	};

}
