using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using NatrixVotingTest1.Models;

namespace NatrixVotingTest1.Interfaces
{
    public interface INatrixVotingTest1Repository
    {
        bool DoesItemExist(string id);
        IEnumerable<QuestionObject> All { get; }
        QuestionObject Find(string id);
        void Insert(QuestionObject qObj);
        void Update(QuestionObject qObj);
        void Delete(string id);
    }
}
