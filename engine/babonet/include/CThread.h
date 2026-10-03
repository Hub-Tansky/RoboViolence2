/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#ifndef CTHREAD_H
#define CTHREAD_H

#include <atomic>
#include <thread>

// Kept for source compatibility: std::thread has no portable priorities, so the value is ignored.
#define CTHREAD_PRIORITY_VERY_LOW 0
#define CTHREAD_PRIORITY_LOW 1
#define CTHREAD_PRIORITY_NORMAL 2
#define CTHREAD_PRIORITY_HIGH 3
#define CTHREAD_PRIORITY_VERY_HIGH 4

// A thread that runs execute(). Derive, implement execute(), call start(). isRunning() turns false once
// execute() has returned; the destructor waits for the thread, so deleting a finished object is safe.
class CThread
{
private:
	// holding the user Data
	void * mArg;

	std::atomic<bool> mIsRunning;
	std::thread mThread;

protected:
	void * arg() const {return mArg;}
	void arg(void * pArg){mArg = pArg;}
	void run(void * arg);

	// The functions you have to redefine
	virtual void setup(); // Setuping your stuff
	virtual void execute(void* pArg) = 0; // Execute your thread

public:
	// Constructor
	CThread();
	virtual ~CThread();

	CThread(const CThread &) = delete;
	CThread & operator=(const CThread &) = delete;

	// To start the thread process. Returns false if it is already running.
	bool start(void * pArg = 0, int pThreadPriority = CTHREAD_PRIORITY_NORMAL);

	// To know if the thread is running
	bool isRunning() const {return mIsRunning;}
};

#endif
