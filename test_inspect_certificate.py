import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class InspectorTests(unittest.TestCase):
    def run_inspector(self, graph='Bg', key='000000000000000500000000000000000000000000000000', witness='3', index=1):
        # Hand-encoded three-vertex path: 0--1--2. Red {0,1}, blue {2}.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'graphs_n3.g6').write_text(graph+'\n')
            (root/'keys_n3.txt').write_text(key+'\n')
            (root/'witnesses_n3.txt').write_text(witness+'\n')
            output=root/'report.html'
            result=subprocess.run([sys.executable,str(Path(__file__).with_name('inspect_certificate.py')),str(root),'--order','3','--index',str(index),'--output',str(output)],capture_output=True,text=True)
            return result,output.read_text() if output.exists() else ''

    def test_exports_verified_record_and_edges(self):
        result,html=self.run_inspector()
        self.assertEqual(result.returncode,0,result.stderr)
        payload=json.loads(html.split('<script id="certificate" type="application/json">')[1].split('</script>')[0])
        self.assertEqual(payload['edges'],[[0,1],[1,2]])
        self.assertEqual(payload['red'],[0,1])
        self.assertEqual(payload['index'],1)

    def test_rejects_graph_key_mismatch(self):
        result,html=self.run_inspector(key='0'*48)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('key/graph mismatch',result.stderr)
        self.assertEqual(html,'')

    def test_rejects_invalid_coloring(self):
        result,html=self.run_inspector(witness='0')
        self.assertNotEqual(result.returncode,0)
        self.assertIn('invalid coloring',result.stderr)
        self.assertEqual(html,'')

    def test_missing_record_does_not_emit_a_verified_report(self):
        result,html=self.run_inspector(index=2)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('record 2',result.stderr)
        self.assertEqual(html,'')

    def test_rejects_nonpositive_index(self):
        result,html=self.run_inspector(index=0)
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(html,'')

if __name__=='__main__':
    unittest.main()
