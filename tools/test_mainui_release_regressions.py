"""Guard the restored native difficulty flow and Witch rage menu copy."""
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class MainUiReleaseRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.proxy = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        cls.editor = (ROOT / 'src/skeeto/core/skeeto_mainui_editor.h').read_text(encoding='utf-8-sig')

    def test_solo_play_does_not_force_expert_but_other_shortcuts_keep_it(self):
        mutation = self.proxy.split('static void MainUiPlayCurrentMutation()', 1)[1].split('static bool MainUiCustomSlider', 1)[0]
        self.assertIn('MainUiClearExpertPending();', mutation)
        self.assertNotIn('MainUiPushExpertDifficulty()', mutation)

        dispatch = self.proxy.split('static void MainUiDispatch(int action)', 1)[1].split('static int g_mainUiFriendVisibleRows', 1)[0]
        solo = dispatch.split('case 108:', 1)[1].split('case 200:', 1)[0]
        self.assertIn('MainUiClearExpertPending();', solo)
        self.assertNotIn('MainUiPushExpertDifficulty()', solo)

        execute = self.editor.split('static void MainUiGameExecute(int action,const char* parameter)', 1)[1].split('static bool MainUiLayoutDispatch', 1)[0]
        self.assertIn('if(action==915)MainUiClearExpertPending();', execute)
        self.assertIn('else if(action==911||action==912||action==913||action==924)MainUiPushExpertDifficulty();', execute)

        consume = self.proxy.split('static void MainUiConsumeExpertPending()', 1)[1].split('static void MainUiPlayCurrentMutation()', 1)[0]
        self.assertIn('if (!ListenIsHost())', consume)
        self.assertIn('g_mainUiExpertPending = 0;', consume)

        self.assertIn('z_difficulty Impossible; map %s coop', self.proxy)
        self.assertIn('coop - - Impossible friends listen', self.proxy)
        self.assertIn('Impossible friends listen', self.editor)
        self.assertIn('z_difficulty Impossible; map %s %s', self.editor)

    def test_witch_rage_hint_describes_rage_sampling(self):
        hint = self.proxy.split('WitchRageHudPositionHint', 1)[1].split('));', 1)[0]
        self.assertIn('怒气', hint)
        self.assertNotIn('检查点伤害', hint)

    def test_quick_test_loadout_only_runs_after_matching_local_map_spawn(self):
        tests = self.proxy.split('static const MainUiFlyItem tests[] = {', 1)[1].split('};', 1)[0]
        maldives = self.proxy.split('static const MainUiFlyItem maldives[] = {', 1)[1].split('};', 1)[0]
        expected = {
            'tumtara': tests,
            'tumtara_l4d1': tests,
            'range_whitaker': tests,
            'range_whitaker_l4d1': tests,
            'range': tests,
            'tgxl': tests,
            'tgxl_night': tests,
            'tankskeeting': tests,
            '0resort_sea_sunset': maldives,
            '0resort_sea_night': maldives,
            '0resort_sea_day': maldives,
            '0resort_beach_day': maldives,
            '0resort_beach_night': maldives,
            '0resort_beach_sunset': maldives,
        }
        for map_id, (map_name, table) in enumerate(expected.items(), 1):
            with self.subTest(map_name=map_name):
                self.assertIn(f'map {map_name}', table)
                self.assertRegex(table, rf'map {map_name}"\s*,nullptr,true,{map_id}\}}')

        map_names = self.proxy.split('static const char* MainUiQuickTestMapName(int id) {', 1)[1].split('static void MainUiQuickTestLoadoutArm', 1)[0]
        expected_level_init_names = list(expected)
        expected_level_init_names[5] = 'tgxl_day'
        for map_id, map_name in enumerate(expected_level_init_names, 1):
            with self.subTest(map_id=map_id):
                self.assertIn(f'case {map_id}: return "{map_name}";', map_names)

        dispatch = self.proxy.split('static void MainUiDispatch(int action)', 1)[1].split('static int g_mainUiFriendVisibleRows', 1)[0]
        quick_click = dispatch.split('if (items[index].console) {', 1)[1].split('char command[64]', 1)[0]
        self.assertIn('MainUiQuickTestLoadoutArm(items[index].quickLoadoutMapId);', quick_click)
        self.assertLess(quick_click.index('MainUiQuickTestLoadoutArm'), quick_click.index('EngClientCmdUnrestricted(items[index].command)'))

        level_init = self.proxy.split('static void MainUiQuickTestLoadoutLevelInit(const char* mapName) {', 1)[1].split('static void MainUiQuickTestLoadoutPump() {', 1)[0]
        self.assertIn('ListenIsHost()', level_init)
        self.assertIn('_stricmp(mapName, expected)', level_init)
        self.assertIn('compare_exchange_strong(state, -state', level_init)

        pump = self.proxy.split('static void MainUiQuickTestLoadoutPump() {', 1)[1].split('static void MainUiPlayCurrentMutation()', 1)[0]
        self.assertIn('g_paintInGame.load', pump)
        self.assertIn('g_dbgLifeState.load', pump)
        self.assertIn('g_dbgLocalTeam.load', pump)
        self.assertIn('g_dbgObsMode.load', pump)
        self.assertIn('compare_exchange_strong', pump)
        for command in (
            'sv_cheats 1', 'give knife', 'give smg_silenced', 'upgrade_add LASER_SIGHT',
            'give first_aid_kit', 'give pain_pills', 'give molotov',
        ):
            with self.subTest(command=command):
                self.assertIn(command, pump)


if __name__ == '__main__':
    unittest.main()
