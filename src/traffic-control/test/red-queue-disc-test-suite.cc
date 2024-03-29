/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright © 2011 Marcos Talau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Marcos Talau (talau@users.sourceforge.net)
 * Modified by:   Pasquale Imputato <p.imputato@gmail.com>
 *
 */

#include "ns3/test.h"
#include "ns3/red-queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

using namespace ns3;

class RedQueueDiscTestItem : public QueueDiscItem {
public:
  RedQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, bool ecnCapable);
  virtual ~RedQueueDiscTestItem ();
  virtual void AddHeader (void);
  virtual bool Mark(void);

private:
  RedQueueDiscTestItem ();
  RedQueueDiscTestItem (const RedQueueDiscTestItem &);
  RedQueueDiscTestItem &operator = (const RedQueueDiscTestItem &);
  bool m_ecnCapablePacket;
};

RedQueueDiscTestItem::RedQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, bool ecnCapable)
  : QueueDiscItem (p, addr, protocol),
    m_ecnCapablePacket (ecnCapable)
{
}

RedQueueDiscTestItem::~RedQueueDiscTestItem ()
{
}

void
RedQueueDiscTestItem::AddHeader (void)
{
}

bool
RedQueueDiscTestItem::Mark (void)
{
  if (m_ecnCapablePacket)
    {
      return true;
    }
  return false;
}

class RedQueueDiscTestCase : public TestCase
{
public:
  RedQueueDiscTestCase ();
  virtual void DoRun (void);
private:
  void Enqueue (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt, bool ecnCapable);
  void RunRedTest (StringValue mode);
};

RedQueueDiscTestCase::RedQueueDiscTestCase ()
  : TestCase ("Sanity check on the red queue implementation")
{
}

void
RedQueueDiscTestCase::RunRedTest (StringValue mode)
{
  uint32_t pktSize = 0;
  // 1 for packets; pktSize for bytes
  uint32_t modeSize = 1;
  double minTh = 2;
  double maxTh = 5;
  uint32_t qSize = 8;
  Ptr<RedQueueDisc> queue = CreateObject<RedQueueDisc> ();

  // test 1: simple enqueue/dequeue with no drops
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");

  Address dest;
  
  if (queue->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      // pktSize should be same as MeanPktSize to avoid performance gap between byte and packet mode
      pktSize = 500;
      modeSize = pktSize;
      queue->SetTh (minTh * modeSize, maxTh * modeSize);
      queue->SetQueueLimit (qSize * modeSize);
    }

  Ptr<Packet> p1, p2, p3, p4, p5, p6, p7, p8;
  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (pktSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);
  p7 = Create<Packet> (pktSize);
  p8 = Create<Packet> (pktSize);

  queue->Initialize ();
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 0 * modeSize, "There should be no packets in there");
  queue->Enqueue (Create<RedQueueDiscTestItem> (p1, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 1 * modeSize, "There should be one packet in there");
  queue->Enqueue (Create<RedQueueDiscTestItem> (p2, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets in there");
  queue->Enqueue (Create<RedQueueDiscTestItem> (p3, dest, 0, false));
  queue->Enqueue (Create<RedQueueDiscTestItem> (p4, dest, 0, false));
  queue->Enqueue (Create<RedQueueDiscTestItem> (p5, dest, 0, false));
  queue->Enqueue (Create<RedQueueDiscTestItem> (p6, dest, 0, false));
  queue->Enqueue (Create<RedQueueDiscTestItem> (p7, dest, 0, false));
  queue->Enqueue (Create<RedQueueDiscTestItem> (p8, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 8 * modeSize, "There should be eight packets in there");

  Ptr<QueueDiscItem> item;

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 7 * modeSize, "There should be seven packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 6 * modeSize, "There should be six packet in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are really no packets in there");


  // test 2: more data, but with no drops
  queue = CreateObject<RedQueueDisc> ();
  minTh = 70 * modeSize;
  maxTh = 150 * modeSize;
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  RedQueueDisc::Stats st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should zero dropped packets due probability mark");
  NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should zero dropped packets due hardmark mark");
  NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should zero dropped packets due queue full");

  // save number of drops from tests
  struct d {
    uint32_t test3;
    uint32_t test4;
    uint32_t test5;
    uint32_t test6;
    uint32_t test7;
    uint32_t test11;
    uint32_t test12;
  } drop;


  // test 3: more data, now drops due QW change
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test3 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_NE (drop.test3, 0, "There should be some dropped packets");


  // test 4: reduced maxTh, this causes more drops
  maxTh = 100 * modeSize;
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test4 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test4, drop.test3, "Test 4 should have more drops than test 3");


  // test 5: change drop probability to a high value (LInterm)
  maxTh = 150 * modeSize;
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (5)), true,
                         "Verify that we can actually set the attribute LInterm");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test5 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test5, drop.test3, "Test 5 should have more drops than test 3");


  // test 6: disable Gentle param
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute Gentle");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test6 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test6, drop.test3, "Test 6 should have more drops than test 3");


  // test 7: disable Wait param
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Wait", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute Wait");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test7 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test7, drop.test3, "Test 7 should have more drops than test 3");


  // test 8: RED queue disc is ECN enabled, but packets are not ECN capable
  queue = CreateObject<RedQueueDisc> ();
  minTh = 30 * modeSize;
  maxTh = 90 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  // Packets are not ECN capable, so there should be only unforced drops, no unforced marks
  NS_TEST_EXPECT_MSG_NE (st.unforcedDrop, 0, "There should be some unforced drops");
  NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced marks");


  // test 9: Packets are ECN capable, but RED queue disc is not ECN enabled
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, true);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  // RED queue disc is not ECN enabled, so there should be only unforced drops, no unforced marks
  NS_TEST_EXPECT_MSG_NE (st.unforcedDrop, 0, "There should be some unforced drops");
  NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced marks");


  // test 10: Packets are ECN capable and RED queue disc is ECN enabled
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, true);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  // Packets are ECN capable, RED queue disc is ECN enabled; there should be only unforced marks, no unforced drops
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced drops");
  NS_TEST_EXPECT_MSG_NE (st.unforcedMark, 0, "There should be some unforced marks");


  // test 11: Original RED with default parameter settings and fixed m_curMaxP
  queue = CreateObject<RedQueueDisc> ();
  minTh = 30 * modeSize;
  maxTh = 90 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test11 = st.unforcedDrop;
  NS_TEST_EXPECT_MSG_NE (drop.test11, 0, "There should some dropped packets due to probability mark");


  // test 12: Feng's Adaptive RED with default parameter settings and varying m_curMaxP
  queue = CreateObject<RedQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("FengAdaptive", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute FengAdaptive");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RedQueueDisc> (queue)->GetStats ();
  drop.test12 = st.unforcedDrop;
  NS_TEST_EXPECT_MSG_LT (drop.test12, drop.test11, "Test 12 should have less drops due to probability mark than test 11");
}

void 
RedQueueDiscTestCase::Enqueue (Ptr<RedQueueDisc> queue, uint32_t size, uint32_t nPkt, bool ecnCapable)
{
  Address dest;
  for (uint32_t i = 0; i < nPkt; i++)
    {
      queue->Enqueue (Create<RedQueueDiscTestItem> (Create<Packet> (size), dest, 0, ecnCapable));
    }
}

void
RedQueueDiscTestCase::DoRun (void)
{
  RunRedTest (StringValue ("QUEUE_MODE_PACKETS"));
  RunRedTest (StringValue ("QUEUE_MODE_BYTES"));
  Simulator::Destroy ();

}

static class RedQueueDiscTestSuite : public TestSuite
{
public:
  RedQueueDiscTestSuite ()
    : TestSuite ("red-queue-disc", UNIT)
  {
    AddTestCase (new RedQueueDiscTestCase (), TestCase::QUICK);
  }
} g_redQueueTestSuite;
