#include "utils/alias/ParticipantRel.hpp"
#include <include/Participant.hpp>
#include <string>

ParticipantID Participant::generateParticipantID(std::string &govID) {
  // govID validity checked by inputHandler (if invalid: Reject Order)
  // check if govID seen already, if not only then participantCount++
  // pCnt_PAN: unique ParticipantID ensured
  ParticipantID pId = std::to_string(Participant::participantCount);
  pId += ("_" + govID);
  return pId;
}
