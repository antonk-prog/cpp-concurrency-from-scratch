#!/usr/bin/env python3
"""Проверяет каноническую структуру курса: обязательные файлы лаб,
непустой starter/, формат шапки task.md. Запускается локально и в CI.
Пустой modules/ не считается ошибкой (курс ещё не сгенерирован)."""
import re
import sys
import pathlib

REQUIRED = [
    "task.md",
    "hints/hint-1.md",
    "hints/hint-2.md",
    "solution/solution.cpp",
    "solution/solution.md",
    "pitfalls.md",
]
TYPES = {"напиши с нуля", "найди и почини", "предскажи вывод",
         "допиши TODO", "нагрузочный вопрос"}
HEADER_RE = re.compile(r"^# Лаба \d+\.\d+\. .+ — (базовая|средняя|сложная)$")

errors = []
modules = sorted(pathlib.Path("modules").glob("module-*"))
if not modules:
    print("modules/ пуст — проверка структуры пропущена (курс не сгенерирован)")
    sys.exit(0)

for mod in modules:
    if not (mod / "README.md").is_file():
        errors.append(f"{mod}: нет README.md (лекция)")
    if not (mod / "PLAN.md").is_file():
        errors.append(f"{mod}: нет PLAN.md")
    labs_dir = mod / "labs"
    labs = sorted(labs_dir.glob("lab-*")) if labs_dir.is_dir() else []
    if not labs:
        errors.append(f"{mod}: нет ни одной лабы")
    for lab in labs:
        for rel in REQUIRED:
            if not (lab / rel).is_file():
                errors.append(f"{lab}: нет {rel}")
        starter = lab / "starter"
        if not starter.is_dir() or not any(starter.iterdir()):
            errors.append(f"{lab}: starter/ пуст или отсутствует")
        task = lab / "task.md"
        if task.is_file():
            lines = task.read_text(encoding="utf-8").splitlines()
            if len(lines) < 3:
                errors.append(f"{lab}: task.md короче трёх строк")
                continue
            if not HEADER_RE.match(lines[0]):
                errors.append(f"{lab}: строка 1 task.md не по формату: {lines[0]!r}")
            if not lines[1].startswith("Модуль: "):
                errors.append(f"{lab}: строка 2 task.md должна начинаться с 'Модуль: '")
            tmatch = re.match(r"^Тип: (.+)$", lines[2])
            if not tmatch or tmatch.group(1).strip() not in TYPES:
                errors.append(f"{lab}: строка 3 task.md — неизвестный тип: {lines[2]!r}")

if errors:
    print("Структурные ошибки:")
    for e in errors:
        print(f"  - {e}")
    sys.exit(1)
n_labs = sum(1 for m in modules for _ in (m / "labs").glob("lab-*")) if all((m / "labs").exists() for m in modules) else 0
print(f"OK: {len(modules)} модулей, структура канонична")
