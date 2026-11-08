#ifndef __RWBENCH_H__
#define __RWBENCH_H__

/*
 * RWBench:  A class to help in the running of benchmarks
 *
 * $Id: bench.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 Rogue Wave Software, Inc.
 * ALL RIGHTS RESERVED
 *
 * The software and information contained herein are proprietary to, and
 * comprise valuable trade secrets of, Rogue Wave Software, Inc., which
 * intends to preserve as trade secrets such software and information.
 * This software is furnished pursuant to a written license agreement and
 * may be used, copied, transmitted, and stored only in accordance with
 * the terms of such license and with the inclusion of the above copyright
 * notice.  This software and information or any other copies thereof may
 * not be provided or otherwise made available to any other person.
 *
 * Notwithstanding any other lease or license that may pertain to, or
 * accompany the delivery of, this computer software and information, the
 * rights of the Government regarding its use, reproduction and disclosure
 * are as set forth in Section 52.227-19 of the FARS Computer
 * Software-Restricted Rights clause.
 * 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause at DFARS 52.227-7013.
 * 
 * This computer software and information is distributed with "restricted
 * rights."  Use, duplication or disclosure is subject to restrictions as
 * set forth in NASA FAR SUP 18-52.227-79 (April 1985) "Commercial
 * Computer Software-Restricted Rights (April 1985)."  If the Clause at
 * 18-52.227-74 "Rights in Data General" is specified in the contract,
 * then the "Alternate III" clause applies.
 *
 ***************************************************************************
 *
 *
 ***************************************************************************
 *
 * To use, derive a class from RWBench including a doLoop(unsigned long) 
 * function, and a what() function (if you plan to use report()).  
 * The doLoop(unsigned long N) function should perform N operations of
 * the type you are trying to benchmark.  RWBench will call this function
 * over and over again until "duration" time has elapsed.  Then it will sum
 * the total number of operations performed.
 *
 * To run, construct an object then call go().  Then call report() to get
 * a summary.  You can call ops() outerLoops(), etc. for more detail.
 *
 * If you wish to correct for overhead, then provide an idleLoop() function
 * which does non-benchmark related calculations.
 *
 ***************************************************************************
 *
 * $Log: bench.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:22  vriezen
 * Move all files to 6.1
 *
 * Revision 2.2  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.2   17 Mar 1992 11:31:22   KEFFER
 * 
 *    Rev 1.0   10 Feb 1992 18:54:06   KEFFER
 * Initial revision.
 */

#include "rw/timer.h"

class RWExport RWBench {

public:

  // Default: 5 second test, one inner loop op, no machine name.
  RWBench(double duration = 5, unsigned long ILO=1000, const char* machine = 0);
  virtual void   parse(int argc, char* argv[]);
  virtual void   go();                      // Run the benchmark
  virtual void   doLoop(unsigned long n)=0; // User supplied: should execute the inner loop n times
  virtual void   idleLoop(unsigned long n); // To calculate looping overhead
  virtual void   what(ostream&) const;      // Print out what we're doing --- used by report()
  void           where(ostream&) const;     // Print out the machine type and the compiler
  virtual void   report(ostream&) const;    // Print out #reps and speed
  double         setDuration(double);       // Set test duration to something else
  unsigned long  setInnerLoops(unsigned long);// Set # inner loops to something else
  double         duration() const           {return timeToTest_;}
  unsigned long  innerLoops() const         {return innerLoops_;}
  double         time() const;              // time to execute in seconds
  unsigned long  outerLoops() const;        // Number of times the inner loop was executed
  double         ops() const;               // Number of operations performed
  double         opsRate() const;           // Number of operations per second
  double         kiloOpsRate() const;       // Number of thousands of ops per second
  double         megaOpsRate() const;       // Number of millions of ops per second

private:

  const char*    machine_;       // What machine we're running on
  double         timeToTest_;    // How long should the test take
  unsigned long  innerLoops_;    // Number of inner loop operations to be done
  unsigned long  outerLoops_;    // Number of outer loops actually executed
  double         delta_;         // Actual time (corrected for overhead)
};

#endif	/* __RWBENCH_H__ */
