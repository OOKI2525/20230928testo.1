# include <Siv3D.hpp>

// ノート
struct Note
{
	// ノートの時刻
	int32 time;

	// 押すべきキーのインデックス (0, 1, 2, 3)
	int32 key;

	// 消えたら false
	bool active = true;
};

// ノート情報を譜面ファイルからロードする関数
Array<Note> LoadNotes(const FilePath& path)
{
	TextReader reader{ path };

	if (not reader)
	{
		throw Error{ U"譜面 {} が見つかりません。"_fmt(path) };
	}

	Array<Note> notes;

	String line;

	// 1 行ずつ読み込む
	while (reader.readLine(line))
	{
		// 空白行はスキップ
		if (line.isEmpty())
		{
			continue;
		}

		// 読み込んだ行を半角スペースで分割
		const Array<String> params = line.split(U' ');

		// 分割した結果が 2 要素でない場合は不正な譜面
		if (params.size() != 2)
		{
			throw Error{ U"不正な譜面です。" };
		}

		// 分割した結果をそれぞれ int32 型に変換
		notes.emplace_back(Parse<int32>(params[0]), Parse<int32>(params[1]));
	}

	return notes;
}

// ノートの座標を計算する関数
Vec2 GetNotePos(const Note& note, int32 time)
{
	const double x = (250 + note.key * 100);
	const double y = (500 - (note.time - time) * 0.25);
	return{ x, y };
}

// ノートを押したときのエフェクト
struct NoteEffect : IEffect
{
	Vec2 m_start;

	int32 m_score;

	Font m_font;

	NoteEffect(const Vec2& start, int32 score, const Font& font)
		: m_start{ start }
		, m_score{ score }
		, m_font{ font } {}

	bool update(double t) override
	{
		Circle{ m_start, (30 + t * 80) }.drawFrame(15 * (0.5 - t));

		if (m_score == 2)
		{
			m_font(U"Excellent").drawAt(32, m_start.movedBy(0, (-20 - t * 160)), Palette::Orange);
		}
		else if (m_score == 1)
		{
			m_font(U"Good").drawAt(32, m_start.movedBy(0, (-20 - t * 160)), Palette::Skyblue);
		}

		return (t < 0.5);
	}
};

void Main()
{
	// ノート配列
	Array<Note> notes = LoadNotes(U"notes.txt");

	// 判定キー
	const Array<Input> Keys = { KeyA, KeyS, KeyD, KeyF };

	// キー入力エフェクトのトランジション
	Array<Transition> keyTransitions(Keys.size(), Transition{ 0.0s, 0.2s });

	// 時間測定用ストップウォッチ
	Stopwatch stopwatch{ StartImmediately::Yes };

	// フォント
	const Font font{ FontMethod::MSDF, 48, Typeface::Heavy };

	// エフェクト管理
	Effect effect;

	while (System::Update())
	{
		// 経過時間（ミリ秒）
		const int32 time = stopwatch.ms();

		ClearPrint();

		Print << time;

		////////////////////////////////
		//
		//	状態更新
		//
		////////////////////////////////

		for (size_t i = 0; i < Keys.size(); ++i)
		{
			keyTransitions[i].update(Keys[i].down());
		}

		for (auto& note : notes)
		{
			// 消えているノートはスキップ
			if (not note.active)
			{
				continue;
			}

			// 現在のタイムとノートのタイムとの差（ミリ秒）
			const int32 diffMillisec = (time - note.time);

			// 差の絶対値が 250 ミリ秒未満なら
			if (Abs(diffMillisec) < 250)
			{
				// ノートに対応するキーが押されていたら
				if (Keys[note.key].down())
				{
					// ノートを消す
					note.active = false;

					// ノートの座標
					const Vec2 notePos = GetNotePos(note, time);

					// エフェクトを追加する
					effect.add<NoteEffect>(Vec2{ notePos.x, 500 }, (Abs(diffMillisec) < 80 ? 2 : 1), font);
				}
			}

			// 250 ミリ秒以上の遅れはミス
			if (note.active && (250 <= diffMillisec))
			{
				// ノートを消す
				note.active = false;
			}
		}

		////////////////////////////////
		//
		//	描画
		//
		////////////////////////////////

		// 入力を描画する
		for (int32 i = 0; i < 4; ++i)
		{
			const double x = (250 + i * 100);
			RectF{ Arg::bottomCenter(x, 600), 80, 600 }
				.draw(Arg::top = ColorF{ 1.0, 0.0 }, Arg::bottom = ColorF{ 1.0, keyTransitions[i].easeOut() * 0.5 });
		}

		// 長方形を描画する
		Rect{ 0, 480, 800, 40 }.draw(ColorF{ 0.5 });

		// キー名を描画する
		for (int32 i = 0; i < 4; ++i)
		{
			const double x = (250 + i * 100);
			font(Keys[i].name()).drawAt(20, Vec2{ x, 500 }, ColorF{ 0.7 });
		}

		// ノートを描画する
		for (const auto& note : notes)
		{
			// 消えているノートはスキップ
			if (not note.active)
			{
				continue;
			}

			// ノートの座標
			const Vec2 notePos = GetNotePos(note, time);

			// 画面内にあるノートのみ描画する
			if (-100.0 < notePos.y)
			{
				Circle{ notePos, 30 }.draw();
			}
		}

		// エフェクトの描画
		effect.update();
	}
}
